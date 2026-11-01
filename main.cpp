#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shellapi.h>
#include <wrl.h>
#include <WebView2.h>
#include <algorithm>
#include <cstdint>
#include <cwchar>
#include <memory>
#include <string>
#include <vector>

using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;

namespace {
constexpr wchar_t kApp[] = L"Microbrowser";
constexpr wchar_t kClass[] = L"MicrobrowserMainWindow";
constexpr wchar_t kDownloadsClass[] = L"MicrobrowserDownloadsWindow";
constexpr int kTop = 82;
constexpr UINT kMaxTabs = 32;
enum : int {
    BACK = 1001, FORWARD, RELOAD, ADDRESS, GO, TAB_BAR,
    TAB_NEW, TAB_CLOSE, PRIVATE_WINDOW, ECO_MODE, DOWNLOADS,
    DOWNLOAD_LIST, DOWNLOAD_CANCEL, DOWNLOAD_FOLDER
};

struct Tab {
    uint64_t id = 0;
    bool closed = false;
    std::wstring url = L"about:blank";
    std::wstring title = L"New tab";
    ComPtr<ICoreWebView2Controller> controller;
    ComPtr<ICoreWebView2> web;
};
struct Download {
    ComPtr<ICoreWebView2DownloadOperation> operation;
    std::wstring path;
};

HWND mainWindow = nullptr, tabBar = nullptr;
HWND backButton = nullptr, forwardButton = nullptr, reloadButton = nullptr;
HWND addressBox = nullptr, goButton = nullptr, newTabButton = nullptr, closeTabButton = nullptr;
HWND privateButton = nullptr, ecoButton = nullptr, downloadsButton = nullptr;
HWND downloadsWindow = nullptr, downloadsList = nullptr;
ComPtr<ICoreWebView2Environment> environment;
std::vector<std::shared_ptr<Tab>> tabs;
std::shared_ptr<Tab> activeTab;
std::vector<std::shared_ptr<Download>> downloadItems;
uint64_t nextTabId = 1;
bool ecoMode = true;
bool privateMode = false;
bool windowMinimized = false;

void Layout();
void ActivateTab(const std::shared_ptr<Tab>& tab);
void StartController(const std::shared_ptr<Tab>& tab);
void UpdateControls();
void UpdateDownloadList();
void NewTab(const std::wstring& url, bool select = true);
bool HandleShortcut(UINT key, bool control, bool shift, bool alt);
std::wstring Trim(const std::wstring& value) {
    size_t a = value.find_first_not_of(L" \t\r\n");
    if (a == std::wstring::npos) return L"";
    size_t b = value.find_last_not_of(L" \t\r\n");
    return value.substr(a, b - a + 1);
}
bool Prefix(const std::wstring& x, const wchar_t* p) {
    return x.size() >= wcslen(p) && _wcsnicmp(x.c_str(), p, wcslen(p)) == 0;
}
bool Allowed(const std::wstring& x) {
    return Prefix(x, L"https://") || Prefix(x, L"http://") || _wcsicmp(x.c_str(), L"about:blank") == 0;
}
std::wstring Encode(const std::wstring& input) {
    int n = WideCharToMultiByte(CP_UTF8, 0, input.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (n <= 0) return L"";
    std::string bytes(static_cast<size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, input.c_str(), -1, bytes.data(), n, nullptr, nullptr);
    constexpr wchar_t hex[] = L"0123456789ABCDEF";
    std::wstring out;
    for (int i = 0; i < n - 1; ++i) {
        unsigned char c = static_cast<unsigned char>(bytes[static_cast<size_t>(i)]);
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
            out += static_cast<wchar_t>(c);
        } else {
            out += L'%';
            out += hex[c >> 4];
            out += hex[c & 15];
        }
    }
    return out;
}
std::wstring Destination(const std::wstring& raw) {
    std::wstring text = Trim(raw);
    if (text.empty()) return L"about:blank";
    if (Allowed(text)) return text;
    bool space = text.find_first_of(L" \t\r\n") != std::wstring::npos;
    bool scheme = text.find(L"://") != std::wstring::npos;
    bool host = text.find(L'.') != std::wstring::npos || Prefix(text, L"localhost") ||
                Prefix(text, L"127.0.0.1") || Prefix(text, L"[::1]");
    if (!space && !scheme && host) {
        if (Prefix(text, L"localhost:") || Prefix(text, L"127.0.0.1:")) return L"http://" + text;
        if (text.find(L':') == std::wstring::npos) return L"https://" + text;
    }
    return L"https://duckduckgo.com/?q=" + Encode(text);
}

std::wstring WindowText(HWND control) {
    if (!control) return L"";
    int size = GetWindowTextLengthW(control);
    std::wstring text(static_cast<size_t>(size) + 1, L'\0');
    GetWindowTextW(control, text.data(), size + 1);
    text.resize(static_cast<size_t>(size));
    return text;
}

int TabIndex(const std::shared_ptr<Tab>& tab) {
    for (size_t i = 0; i < tabs.size(); ++i) {
        if (tabs[i] == tab) return static_cast<int>(i);
    }
    return -1;
}

std::wstring ShortTitle(const std::wstring& value) {
    if (value.size() <= 20) return value;
    return value.substr(0, 19) + L"...";
}

void UpdateTabTitle(const std::shared_ptr<Tab>& tab) {
    int index = TabIndex(tab);
    if (index < 0 || !tabBar) return;
    std::wstring label = ShortTitle(tab->title.empty() ? L"New tab" : tab->title);
    TCITEMW item{};
    item.mask = TCIF_TEXT;
    item.pszText = label.data();
    TabCtrl_SetItem(tabBar, index, &item);
}

void UpdateControls() {
    BOOL canBack = FALSE, canForward = FALSE;
    ComPtr<ICoreWebView2> web = activeTab ? activeTab->web : nullptr;
    if (web) {
        web->get_CanGoBack(&canBack);
        web->get_CanGoForward(&canForward);
    }
    if (backButton) EnableWindow(backButton, canBack);
    if (forwardButton) EnableWindow(forwardButton, canForward);
    if (reloadButton) EnableWindow(reloadButton, web != nullptr);
    if (goButton) EnableWindow(goButton, web != nullptr);
    if (closeTabButton) EnableWindow(closeTabButton, !tabs.empty());
    if (ecoButton) SetWindowTextW(ecoButton, ecoMode ? L"Eco: ON" : L"Eco: OFF");
}

void PutVisibility(const std::shared_ptr<Tab>& tab, bool visible) {
    if (!tab || !tab->controller) return;
    tab->controller->put_IsVisible(visible ? TRUE : FALSE);
    if (!tab->web) return;
    ComPtr<ICoreWebView2_3> advanced;
    if (FAILED(tab->web.As(&advanced)) || !advanced) return;
    if (visible) {
        advanced->Resume();
    } else if (ecoMode) {
        advanced->TrySuspend(
            Callback<ICoreWebView2TrySuspendCompletedHandler>(
                [](HRESULT, BOOL) -> HRESULT { return S_OK; }).Get());
    } else {
        advanced->Resume();
    }
}

void ActivateTab(const std::shared_ptr<Tab>& tab) {
    if (!tab || tab->closed || TabIndex(tab) < 0) return;
    activeTab = tab;
    int index = TabIndex(tab);
    if (tabBar && TabCtrl_GetCurSel(tabBar) != index) TabCtrl_SetCurSel(tabBar, index);

    for (const auto& entry : tabs)
        PutVisibility(entry, entry == tab && !windowMinimized);

    if (addressBox)
        SetWindowTextW(addressBox, tab->url == L"about:blank" ? L"" : tab->url.c_str());
    UpdateControls();
    Layout();
    if (mainWindow) {
        SetWindowTextW(mainWindow, privateMode ? L"Microbrowser - Private" : L"Microbrowser");
    }
}

void Navigate() {
    if (!activeTab || !activeTab->web) return;
    std::wstring url = Destination(WindowText(addressBox));
    activeTab->web->Navigate(url.c_str());
    if (activeTab->controller)
        activeTab->controller->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
}

void Layout() {
    if (!mainWindow) return;
    RECT bounds{};
    GetClientRect(mainWindow, &bounds);
    int width = std::max(0L, bounds.right - bounds.left);
    int height = std::max(0L, bounds.bottom - bounds.top);

    int stripWidth = std::max(120, width - 316);
    if (tabBar) MoveWindow(tabBar, 3, 3, stripWidth, 34, TRUE);
    int x = stripWidth + 7;
    for (HWND control : {newTabButton, closeTabButton}) {
        if (control) MoveWindow(control, x, 3, 35, 30, TRUE);
        x += 39;
    }
    if (privateButton) MoveWindow(privateButton, x, 3, 74, 30, TRUE);
    x += 78;
    if (ecoButton) MoveWindow(ecoButton, x, 3, 70, 30, TRUE);
    x += 74;
    if (downloadsButton) MoveWindow(downloadsButton, x, 3, 78, 30, TRUE);

    x = 6;
    for (HWND control : {backButton, forwardButton, reloadButton}) {
        if (control) MoveWindow(control, x, 42, 39, 32, TRUE);
        x += 44;
    }
    int addressWidth = std::max(20, width - x - 59);
    if (addressBox) MoveWindow(addressBox, x, 42, addressWidth, 32, TRUE);
    if (goButton) MoveWindow(goButton, x + addressWidth + 5, 42, 45, 32, TRUE);

    RECT webBounds{0, kTop, width, height};
    for (const auto& tab : tabs) {
        if (tab->controller) tab->controller->put_Bounds(webBounds);
    }
}

std::wstring ProfileDirectory() {
    PWSTR local = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &local))) return L"";
    std::wstring dir = std::wstring(local) + L"\\Microbrowser";
    CoTaskMemFree(local);
    CreateDirectoryW(dir.c_str(), nullptr);
    dir += L"\\Profile";
    CreateDirectoryW(dir.c_str(), nullptr);
    return dir;
}

void ShowError(const wchar_t* stage, HRESULT result) {
    wchar_t message[360]{};
    swprintf_s(message, L"WebView2 failed at %s (0x%08X).\nInstall or update Microsoft Edge WebView2 Runtime.",
               stage, static_cast<unsigned int>(result));
    if (IsWindow(mainWindow)) MessageBoxW(mainWindow, message, L"Microbrowser", MB_OK | MB_ICONERROR);
}

std::wstring DownloadLabel(const std::shared_ptr<Download>& entry) {
    if (!entry || !entry->operation) return L"Unknown download";
    size_t sep = entry->path.find_last_of(L"\\/");
    std::wstring filename = sep == std::wstring::npos ? entry->path : entry->path.substr(sep + 1);
    if (filename.empty()) filename = L"Download";
    COREWEBVIEW2_DOWNLOAD_STATE state{};
    entry->operation->get_State(&state);
    if (state == COREWEBVIEW2_DOWNLOAD_STATE_COMPLETED) return filename + L"  |  Complete";
    if (state == COREWEBVIEW2_DOWNLOAD_STATE_INTERRUPTED) return filename + L"  |  Interrupted / cancelled";

    INT64 received = 0;
    INT64 total = -1;
    entry->operation->get_BytesReceived(&received);
    entry->operation->get_TotalBytesToReceive(&total);
    std::wstring label = filename + L"  |  " + std::to_wstring(received / 1024) + L" KB";
    if (total > 0) {
        label += L" / " + std::to_wstring(total / 1024) + L" KB";
        label += L" (" + std::to_wstring(received * 100 / total) + L"%)";
    }
    return label;
}

void UpdateDownloadList() {
    if (!downloadsList || !IsWindow(downloadsList)) return;
    LRESULT selection = SendMessageW(downloadsList, LB_GETCURSEL, 0, 0);
    SendMessageW(downloadsList, LB_RESETCONTENT, 0, 0);
    for (const auto& entry : downloadItems) {
        std::wstring label = DownloadLabel(entry);
        SendMessageW(downloadsList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
    }
    if (selection != LB_ERR && selection < static_cast<LRESULT>(downloadItems.size()))
        SendMessageW(downloadsList, LB_SETCURSEL, static_cast<WPARAM>(selection), 0);
}

void ShowDownloads() {
    if (downloadsWindow && IsWindow(downloadsWindow)) {
        ShowWindow(downloadsWindow, SW_RESTORE);
        SetForegroundWindow(downloadsWindow);
        return;
    }
    downloadsWindow = CreateWindowExW(WS_EX_APPWINDOW, kDownloadsClass, L"Microbrowser - Downloads",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 640, 330,
        mainWindow, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (!downloadsWindow) return;
    ShowWindow(downloadsWindow, SW_SHOWNORMAL);
    UpdateDownloadList();
}

void OpenDownloadFolder() {
    if (!downloadsList) return;
    LRESULT index = SendMessageW(downloadsList, LB_GETCURSEL, 0, 0);
    if (index == LB_ERR || index < 0 || static_cast<size_t>(index) >= downloadItems.size()) return;
    std::wstring path = downloadItems[static_cast<size_t>(index)]->path;
    size_t sep = path.find_last_of(L"\\/");
    if (sep == std::wstring::npos) return;
    std::wstring dir = path.substr(0, sep);
    ShellExecuteW(downloadsWindow, L"open", dir.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

void TrackDownload(ICoreWebView2DownloadStartingEventArgs* args) {
    if (!args) return;
    ComPtr<ICoreWebView2DownloadOperation> operation;
    if (FAILED(args->get_DownloadOperation(&operation)) || !operation) return;
    auto item = std::make_shared<Download>();
    item->operation = operation;
    LPWSTR path = nullptr;
    if (SUCCEEDED(operation->get_ResultFilePath(&path)) && path) {
        item->path = path;
        CoTaskMemFree(path);
    }
    downloadItems.push_back(item);
    args->put_Handled(TRUE);
    EventRegistrationToken token{};
    operation->add_BytesReceivedChanged(
        Callback<ICoreWebView2BytesReceivedChangedEventHandler>(
            [](ICoreWebView2DownloadOperation*, IUnknown*) -> HRESULT {
                UpdateDownloadList();
                return S_OK;
            }).Get(), &token);
    operation->add_StateChanged(
        Callback<ICoreWebView2StateChangedEventHandler>(
            [](ICoreWebView2DownloadOperation*, IUnknown*) -> HRESULT {
                UpdateDownloadList();
                return S_OK;
            }).Get(), &token);
    UpdateDownloadList();
}

void RefreshTabUrl(const std::shared_ptr<Tab>& tab) {
    if (!tab || tab->closed || !tab->web) return;
    LPWSTR text = nullptr;
    if (SUCCEEDED(tab->web->get_Source(&text)) && text) {
        tab->url = text;
        CoTaskMemFree(text);
    }
    if (activeTab == tab && addressBox && GetFocus() != addressBox)
        SetWindowTextW(addressBox, tab->url == L"about:blank" ? L"" : tab->url.c_str());
    UpdateControls();
}

void RegisterWebEvents(const std::shared_ptr<Tab>& tab) {
    if (!tab || !tab->web) return;
    std::weak_ptr<Tab> weak = tab;
    ComPtr<ICoreWebView2Settings> settings;
    if (SUCCEEDED(tab->web->get_Settings(&settings)) && settings) {
        settings->put_IsWebMessageEnabled(FALSE);
        settings->put_AreHostObjectsAllowed(FALSE);
        settings->put_IsStatusBarEnabled(TRUE);
    }
    EventRegistrationToken token{};
    tab->web->add_SourceChanged(
        Callback<ICoreWebView2SourceChangedEventHandler>(
            [weak](ICoreWebView2*, ICoreWebView2SourceChangedEventArgs*) -> HRESULT {
                auto tab = weak.lock();
                if (tab && !tab->closed) RefreshTabUrl(tab);
                return S_OK;
            }).Get(), &token);
    tab->web->add_DocumentTitleChanged(
        Callback<ICoreWebView2DocumentTitleChangedEventHandler>(
            [weak](ICoreWebView2*, IUnknown*) -> HRESULT {
                auto tab = weak.lock();
                if (!tab || tab->closed || !tab->web) return S_OK;
                LPWSTR title = nullptr;
                if (SUCCEEDED(tab->web->get_DocumentTitle(&title)) && title) {
                    tab->title = *title ? title : L"New tab";
                    CoTaskMemFree(title);
                    UpdateTabTitle(tab);
                }
                return S_OK;
            }).Get(), &token);
    tab->web->add_HistoryChanged(
        Callback<ICoreWebView2HistoryChangedEventHandler>(
            [weak](ICoreWebView2*, IUnknown*) -> HRESULT {
                if (auto tab = weak.lock(); tab && activeTab == tab) UpdateControls();
                return S_OK;
            }).Get(), &token);
    tab->web->add_NavigationStarting(
        Callback<ICoreWebView2NavigationStartingEventHandler>(
            [](ICoreWebView2*, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT {
                LPWSTR uri = nullptr;
                bool valid = SUCCEEDED(args->get_Uri(&uri)) && uri && Allowed(uri);
                if (uri) CoTaskMemFree(uri);
                if (!valid) args->put_Cancel(TRUE);
                return S_OK;
            }).Get(), &token);
    tab->web->add_PermissionRequested(
        Callback<ICoreWebView2PermissionRequestedEventHandler>(
            [](ICoreWebView2*, ICoreWebView2PermissionRequestedEventArgs* args) -> HRESULT {
                COREWEBVIEW2_PERMISSION_KIND kind{};
                if (SUCCEEDED(args->get_PermissionKind(&kind)) &&
                    (kind == COREWEBVIEW2_PERMISSION_KIND_CAMERA ||
                     kind == COREWEBVIEW2_PERMISSION_KIND_MICROPHONE ||
                     kind == COREWEBVIEW2_PERMISSION_KIND_GEOLOCATION ||
                     kind == COREWEBVIEW2_PERMISSION_KIND_NOTIFICATIONS))
                    args->put_State(COREWEBVIEW2_PERMISSION_STATE_DENY);
                return S_OK;
            }).Get(), &token);
    tab->web->add_NewWindowRequested(
        Callback<ICoreWebView2NewWindowRequestedEventHandler>(
            [](ICoreWebView2*, ICoreWebView2NewWindowRequestedEventArgs* args) -> HRESULT {
                args->put_Handled(TRUE);
                LPWSTR uri = nullptr;
                if (SUCCEEDED(args->get_Uri(&uri)) && uri) {
                    if (Allowed(uri)) NewTab(uri);
                    CoTaskMemFree(uri);
                }
                return S_OK;
            }).Get(), &token);

    ComPtr<ICoreWebView2_4> web4;
    if (SUCCEEDED(tab->web.As(&web4)) && web4) {
        web4->add_DownloadStarting(
            Callback<ICoreWebView2DownloadStartingEventHandler>(
                [](ICoreWebView2*, ICoreWebView2DownloadStartingEventArgs* args) -> HRESULT {
                    TrackDownload(args);
                    return S_OK;
                }).Get(), &token);
    }
    tab->controller->add_AcceleratorKeyPressed(
        Callback<ICoreWebView2AcceleratorKeyPressedEventHandler>(
            [](ICoreWebView2Controller*, ICoreWebView2AcceleratorKeyPressedEventArgs* args) -> HRESULT {
                COREWEBVIEW2_KEY_EVENT_KIND kind{};
                UINT key = 0;
                args->get_KeyEventKind(&kind);
                args->get_VirtualKey(&key);
                if (kind != COREWEBVIEW2_KEY_EVENT_KIND_KEY_DOWN &&
                    kind != COREWEBVIEW2_KEY_EVENT_KIND_SYSTEM_KEY_DOWN)
                    return S_OK;
                bool control = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
                bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                bool alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
                if (HandleShortcut(key, control, shift, alt)) args->put_Handled(TRUE);
                return S_OK;
            }).Get(), &token);
}

void StartController(const std::shared_ptr<Tab>& tab) {
    if (!environment || !tab || tab->closed || tab->controller || !IsWindow(mainWindow)) return;
    std::weak_ptr<Tab> weak = tab;
    auto onCreated = Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
        [weak](HRESULT result, ICoreWebView2Controller* created) -> HRESULT {
            auto tab = weak.lock();
            if (!tab || tab->closed || !IsWindow(mainWindow)) {
                if (created) created->Close();
                return S_OK;
            }
            if (FAILED(result) || !created) {
                ShowError(L"controller", result);
                return S_OK;
            }
            tab->controller = created;
            if (FAILED(created->get_CoreWebView2(&tab->web)) || !tab->web) {
                ShowError(L"webview", E_FAIL);
                return S_OK;
            }
            RegisterWebEvents(tab);
            Layout();
            PutVisibility(tab, tab == activeTab && !windowMinimized);
            tab->web->Navigate(tab->url.c_str());
            UpdateControls();
            return S_OK;
        });

    HRESULT result = E_NOINTERFACE;
    if (privateMode) {
        ComPtr<ICoreWebView2Environment10> env10;
        if (FAILED(environment.As(&env10)) || !env10) {
            MessageBoxW(mainWindow, L"Private mode requires a newer WebView2 Runtime. It will not fall back to normal mode.",
                        kApp, MB_OK | MB_ICONERROR);
            return;
        }
        ComPtr<ICoreWebView2ControllerOptions> options;
        result = env10->CreateCoreWebView2ControllerOptions(&options);
        if (SUCCEEDED(result) && options) result = options->put_IsInPrivateModeEnabled(TRUE);
        if (SUCCEEDED(result)) result = env10->CreateCoreWebView2ControllerWithOptions(
            mainWindow, options.Get(), onCreated.Get());
    } else {
        result = environment->CreateCoreWebView2Controller(mainWindow, onCreated.Get());
    }
    if (FAILED(result)) ShowError(privateMode ? L"private controller" : L"controller", result);
}

void NewTab(const std::wstring& url, bool select) {
    if (tabs.size() >= kMaxTabs) {
        MessageBoxW(mainWindow, L"Maximum of 32 tabs reached.", kApp, MB_OK | MB_ICONINFORMATION);
        return;
    }
    auto tab = std::make_shared<Tab>();
    tab->id = nextTabId++;
    tab->url = url;
    tabs.push_back(tab);
    if (tabBar) {
        std::wstring label = L"New tab";
        TCITEMW item{};
        item.mask = TCIF_TEXT;
        item.pszText = label.data();
        TabCtrl_InsertItem(tabBar, static_cast<int>(tabs.size() - 1), &item);
    }
    if (select) ActivateTab(tab);
    if (environment) StartController(tab);
}

void CloseActiveTab() {
    if (!activeTab) return;
    int index = TabIndex(activeTab);
    if (index < 0) return;
    auto closing = activeTab;
    closing->closed = true;
    activeTab.reset();
    tabs.erase(tabs.begin() + index);
    if (tabBar) TabCtrl_DeleteItem(tabBar, index);
    if (closing->controller) closing->controller->Close();
    closing->web.Reset();
    closing->controller.Reset();
    if (tabs.empty()) {
        NewTab(L"about:blank");
    } else {
        ActivateTab(tabs[static_cast<size_t>(std::min(index, static_cast<int>(tabs.size()) - 1))]);
    }
}

void LaunchPrivateWindow() {
    wchar_t exe[MAX_PATH]{};
    DWORD count = GetModuleFileNameW(nullptr, exe, MAX_PATH);
    if (!count || count >= MAX_PATH) return;
    std::wstring command = L"\"" + std::wstring(exe) + L"\" --private";
    std::vector<wchar_t> mutableCommand(command.begin(), command.end());
    mutableCommand.push_back(L'\0');
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(exe, mutableCommand.data(), nullptr, nullptr, FALSE, 0,
                        nullptr, nullptr, &startup, &process)) {
        MessageBoxW(mainWindow, L"Unable to open private window.", kApp, MB_OK | MB_ICONERROR);
        return;
    }
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
}

bool HandleShortcut(UINT key, bool control, bool shift, bool alt) {
    if (control && key == 'L') {
        SetFocus(addressBox);
        SendMessageW(addressBox, EM_SETSEL, 0, -1);
        return true;
    }
    if (control && shift && key == 'N') { LaunchPrivateWindow(); return true; }
    if (control && key == 'T') { NewTab(L"about:blank"); return true; }
    if (control && key == 'W') { CloseActiveTab(); return true; }
    if (control && key == VK_TAB && !tabs.empty()) {
        int index = TabIndex(activeTab);
        index += shift ? static_cast<int>(tabs.size()) - 1 : 1;
        ActivateTab(tabs[static_cast<size_t>(index % static_cast<int>(tabs.size()))]);
        return true;
    }
    if (control && key == 'J') { ShowDownloads(); return true; }
    if (key == VK_F5 || (control && key == 'R')) {
        if (activeTab && activeTab->web) activeTab->web->Reload();
        return true;
    }
    if (alt && key == VK_LEFT) {
        if (activeTab && activeTab->web) activeTab->web->GoBack();
        return true;
    }
    if (alt && key == VK_RIGHT) {
        if (activeTab && activeTab->web) activeTab->web->GoForward();
        return true;
    }
    return false;
}

void InitBrowserEnvironment() {
    std::wstring profile = ProfileDirectory();
    if (profile.empty()) {
        MessageBoxW(mainWindow, L"Cannot create browser profile.", kApp, MB_OK | MB_ICONERROR);
        return;
    }
    HRESULT result = CreateCoreWebView2EnvironmentWithOptions(
        nullptr, profile.c_str(), nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [](HRESULT result, ICoreWebView2Environment* created) -> HRESULT {
                if (!IsWindow(mainWindow)) return S_OK;
                if (FAILED(result) || !created) {
                    ShowError(L"environment", result);
                    return S_OK;
                }
                environment = created;
                for (const auto& tab : tabs) StartController(tab);
                return S_OK;
            }).Get());
    if (FAILED(result)) ShowError(L"environment request", result);
}

LRESULT CALLBACK EditProc(HWND control, UINT message, WPARAM wp, LPARAM lp, UINT_PTR, DWORD_PTR) {
    if (message == WM_KEYDOWN && wp == VK_RETURN) {
        Navigate();
        return 0;
    }
    if (message == WM_KEYDOWN && wp == VK_ESCAPE) {
        if (activeTab) SetWindowTextW(control,
            activeTab->url == L"about:blank" ? L"" : activeTab->url.c_str());
        if (activeTab && activeTab->controller)
            activeTab->controller->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
        return 0;
    }
    if (message == WM_KEYDOWN) {
        bool controlKey = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        bool shiftKey = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        bool altKey = (GetKeyState(VK_MENU) & 0x8000) != 0;
        if (HandleShortcut(static_cast<UINT>(wp), controlKey, shiftKey, altKey)) return 0;
    }
    return DefSubclassProc(control, message, wp, lp);
}

LRESULT CALLBACK DownloadsProc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp) {
    switch (message) {
    case WM_CREATE: {
        downloadsList = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY, 10, 10, 600, 220,
            hwnd, reinterpret_cast<HMENU>(DOWNLOAD_LIST), nullptr, nullptr);
        HWND cancelButton = CreateWindowW(L"BUTTON", L"Cancel selected", WS_CHILD | WS_VISIBLE,
            10, 240, 140, 31, hwnd, reinterpret_cast<HMENU>(DOWNLOAD_CANCEL), nullptr, nullptr);
        HWND folderButton = CreateWindowW(L"BUTTON", L"Open folder", WS_CHILD | WS_VISIBLE,
            160, 240, 140, 31, hwnd, reinterpret_cast<HMENU>(DOWNLOAD_FOLDER), nullptr, nullptr);
        HFONT font = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        for (HWND child : {downloadsList, cancelButton, folderButton})
            SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        return 0;
    }
    case WM_SIZE: {
        RECT r{};
        GetClientRect(hwnd, &r);
        int width = std::max(300L, r.right - r.left);
        int height = std::max(180L, r.bottom - r.top);
        if (downloadsList) MoveWindow(downloadsList, 10, 10, width - 20, height - 62, TRUE);
        if (HWND cancelButton = GetDlgItem(hwnd, DOWNLOAD_CANCEL))
            MoveWindow(cancelButton, 10, height - 42, 140, 30, TRUE);
        if (HWND folderButton = GetDlgItem(hwnd, DOWNLOAD_FOLDER))
            MoveWindow(folderButton, 160, height - 42, 140, 30, TRUE);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wp) == DOWNLOAD_CANCEL) {
            LRESULT index = SendMessageW(downloadsList, LB_GETCURSEL, 0, 0);
            if (index != LB_ERR && index >= 0 && static_cast<size_t>(index) < downloadItems.size()) {
                auto item = downloadItems[static_cast<size_t>(index)];
                if (item->operation) item->operation->Cancel();
                UpdateDownloadList();
            }
        } else if (LOWORD(wp) == DOWNLOAD_FOLDER) {
            OpenDownloadFolder();
        }
        return 0;
    case WM_DESTROY:
        downloadsWindow = nullptr;
        downloadsList = nullptr;
        return 0;
    default:
        return DefWindowProcW(hwnd, message, wp, lp);
    }
}

LRESULT CALLBACK MainProc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp) {
    switch (message) {
    case WM_CREATE: {
        mainWindow = hwnd;
        DWORD buttonStyle = WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON;
        tabBar = CreateWindowExW(0, WC_TABCONTROLW, L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | TCS_FOCUSNEVER | TCS_SINGLELINE,
            0, 0, 0, 0, hwnd, reinterpret_cast<HMENU>(TAB_BAR), nullptr, nullptr);
        auto button = [&](int id, const wchar_t* caption) {
            return CreateWindowW(L"BUTTON", caption, buttonStyle, 0, 0, 0, 0,
                hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), nullptr, nullptr);
        };
        newTabButton = button(TAB_NEW, L"+");
        closeTabButton = button(TAB_CLOSE, L"x");
        privateButton = button(PRIVATE_WINDOW, L"Private");
        ecoButton = button(ECO_MODE, L"Eco: ON");
        downloadsButton = button(DOWNLOADS, L"Downloads");
        backButton = button(BACK, L"<");
        forwardButton = button(FORWARD, L">");
        reloadButton = button(RELOAD, L"R");
        addressBox = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 0, 0, 0, 0,
            hwnd, reinterpret_cast<HMENU>(ADDRESS), nullptr, nullptr);
        goButton = button(GO, L"Go");
        HFONT font = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        for (HWND control : {tabBar, newTabButton, closeTabButton, privateButton,
             ecoButton, downloadsButton, backButton, forwardButton, reloadButton,
             addressBox, goButton}) {
            SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        }
        SetWindowSubclass(addressBox, EditProc, 1, 0);
        NewTab(L"about:blank");
        UpdateControls();
        return 0;
    }
    case WM_SIZE:
        if (wp == SIZE_MINIMIZED) {
            windowMinimized = true;
            for (const auto& tab : tabs) PutVisibility(tab, false);
        } else {
            windowMinimized = false;
            for (const auto& tab : tabs) PutVisibility(tab, tab == activeTab);
            Layout();
        }
        return 0;
    case WM_NOTIFY: {
        auto* header = reinterpret_cast<NMHDR*>(lp);
        if (header && header->idFrom == TAB_BAR && header->code == TCN_SELCHANGE) {
            int index = TabCtrl_GetCurSel(tabBar);
            if (index >= 0 && static_cast<size_t>(index) < tabs.size()) ActivateTab(tabs[static_cast<size_t>(index)]);
        }
        return 0;
    }
    case WM_COMMAND:
        if (HIWORD(wp) == BN_CLICKED) {
            switch (LOWORD(wp)) {
            case BACK: if (activeTab && activeTab->web) activeTab->web->GoBack(); return 0;
            case FORWARD: if (activeTab && activeTab->web) activeTab->web->GoForward(); return 0;
            case RELOAD: if (activeTab && activeTab->web) activeTab->web->Reload(); return 0;
            case GO: Navigate(); return 0;
            case TAB_NEW: NewTab(L"about:blank"); return 0;
            case TAB_CLOSE: CloseActiveTab(); return 0;
            case PRIVATE_WINDOW: LaunchPrivateWindow(); return 0;
            case ECO_MODE:
                ecoMode = !ecoMode;
                for (const auto& tab : tabs) PutVisibility(tab, tab == activeTab && !windowMinimized);
                UpdateControls();
                return 0;
            case DOWNLOADS: ShowDownloads(); return 0;
            }
        }
        return 0;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN: {
        bool control = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        bool alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
        if (HandleShortcut(static_cast<UINT>(wp), control, shift, alt)) return 0;
        break;
    }
    case WM_GETMINMAXINFO: {
        auto* minMax = reinterpret_cast<MINMAXINFO*>(lp);
        minMax->ptMinTrackSize.x = 760;
        minMax->ptMinTrackSize.y = 340;
        return 0;
    }
    case WM_DESTROY:
        if (downloadsWindow && IsWindow(downloadsWindow)) DestroyWindow(downloadsWindow);
        for (const auto& tab : tabs) {
            tab->closed = true;
            if (tab->controller) tab->controller->Close();
            tab->web.Reset();
            tab->controller.Reset();
        }
        tabs.clear();
        activeTab.reset();
        downloadItems.clear();
        environment.Reset();
        mainWindow = nullptr;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, message, wp, lp);
}
}  // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR cmdLine, int show) {
    privateMode = cmdLine && wcsstr(cmdLine, L"--private") != nullptr;
    SetProcessDPIAware();
    HRESULT init = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(init)) {
        MessageBoxW(nullptr, L"COM initialization failed.", kApp, MB_OK | MB_ICONERROR);
        return 1;
    }
    INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&controls);

    WNDCLASSW downloadsClass{};
    downloadsClass.hInstance = instance;
    downloadsClass.lpszClassName = kDownloadsClass;
    downloadsClass.lpfnWndProc = DownloadsProc;
    downloadsClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    downloadsClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    RegisterClassW(&downloadsClass);

    WNDCLASSW cls{};
    cls.hInstance = instance;
    cls.lpszClassName = kClass;
    cls.lpfnWndProc = MainProc;
    cls.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    cls.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    if (!RegisterClassW(&cls)) {
        CoUninitialize();
        return 1;
    }
    HWND hwnd = CreateWindowExW(0, kClass,
        privateMode ? L"Microbrowser - Private" : L"Microbrowser",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1100, 780,
        nullptr, nullptr, instance, nullptr);
    if (!hwnd) {
        CoUninitialize();
        return 1;
    }
    ShowWindow(hwnd, show);
    UpdateWindow(hwnd);
    InitBrowserEnvironment();
    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    CoUninitialize();
    return static_cast<int>(msg.wParam);
}
