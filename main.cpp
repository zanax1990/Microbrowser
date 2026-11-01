#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <wrl.h>
#include <WebView2.h>
#include <algorithm>
#include <cwctype>
#include <string>

using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;

namespace {
constexpr wchar_t kApp[] = L"Microbrowser";
constexpr wchar_t kClass[] = L"MicrobrowserMainWindow";
enum : int { BACK = 1001, FORWARD, RELOAD, ADDRESS, GO };
HWND window = nullptr, back = nullptr, forward = nullptr, reload = nullptr, address = nullptr, go = nullptr;
ComPtr<ICoreWebView2Controller> controller;
ComPtr<ICoreWebView2> browser;

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
void RefreshButtons() {
    BOOL canBack = FALSE, canForward = FALSE;
    if (browser) {
        browser->get_CanGoBack(&canBack);
        browser->get_CanGoForward(&canForward);
    }
    if (back) EnableWindow(back, canBack);
    if (forward) EnableWindow(forward, canForward);
    if (reload) EnableWindow(reload, browser != nullptr);
    if (go) EnableWindow(go, browser != nullptr);
}
void Navigate() {
    if (!browser || !address) return;
    int length = GetWindowTextLengthW(address);
    std::wstring value(static_cast<size_t>(length) + 1, L'\0');
    GetWindowTextW(address, value.data(), length + 1);
    value.resize(static_cast<size_t>(length));
    std::wstring url = Destination(value);
    browser->Navigate(url.c_str());
    if (controller) controller->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
}
void Layout() {
    if (!window) return;
    RECT r{};
    GetClientRect(window, &r);
    int w = std::max(0L, r.right - r.left);
    int h = std::max(0L, r.bottom - r.top);
    int x = 5;
    for (HWND control : {back, forward, reload}) {
        if (control) MoveWindow(control, x, 5, 39, 32, TRUE);
        x += 44;
    }
    int addressWidth = std::max(20, w - x - 53);
    if (address) MoveWindow(address, x, 5, addressWidth, 32, TRUE);
    if (go) MoveWindow(go, x + addressWidth + 5, 5, 43, 32, TRUE);
    if (controller) {
        RECT bounds{0, 43, w, h};
        controller->put_Bounds(bounds);
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
void Error(const wchar_t* step, HRESULT hr) {
    wchar_t message[300]{};
    swprintf_s(message, L"WebView2 failed at %s (0x%08X).\nInstall or update Microsoft Edge WebView2 Runtime.",
               step, static_cast<unsigned int>(hr));
    MessageBoxW(window, message, L"Microbrowser error", MB_OK | MB_ICONERROR);
}
void InitBrowser() {
    std::wstring profile = ProfileDirectory();
    if (profile.empty()) {
        MessageBoxW(window, L"Cannot create browser profile.", kApp, MB_OK | MB_ICONERROR);
        return;
    }
    HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
        nullptr, profile.c_str(), nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [](HRESULT result, ICoreWebView2Environment* environment) -> HRESULT {
                if (FAILED(result) || !environment) {
                    Error(L"environment", result);
                    return S_OK;
                }
                if (!IsWindow(window)) return S_OK;
                HRESULT requested = environment->CreateCoreWebView2Controller(
                    window,
                    Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [](HRESULT result, ICoreWebView2Controller* created) -> HRESULT {
                            if (FAILED(result) || !created) {
                                Error(L"controller", result);
                                return S_OK;
                            }
                            if (!IsWindow(window)) {
                                created->Close();
                                return S_OK;
                            }
                            controller = created;
                            if (FAILED(controller->get_CoreWebView2(&browser)) || !browser) {
                                Error(L"webview", E_FAIL);
                                return S_OK;
                            }
                            ComPtr<ICoreWebView2Settings> settings;
                            if (SUCCEEDED(browser->get_Settings(&settings)) && settings) {
                                settings->put_IsWebMessageEnabled(FALSE);
                                settings->put_AreHostObjectsAllowed(FALSE);
                                settings->put_IsStatusBarEnabled(TRUE);
                            }
                            EventRegistrationToken token{};
                            browser->add_SourceChanged(
                                Callback<ICoreWebView2SourceChangedEventHandler>(
                                    [](ICoreWebView2*, ICoreWebView2SourceChangedEventArgs*) -> HRESULT {
                                        if (!browser || !address) return S_OK;
                                        LPWSTR src = nullptr;
                                        if (SUCCEEDED(browser->get_Source(&src)) && src) {
                                            if (GetFocus() != address)
                                                SetWindowTextW(address, _wcsicmp(src, L"about:blank") == 0 ? L"" : src);
                                            CoTaskMemFree(src);
                                        }
                                        RefreshButtons();
                                        return S_OK;
                                    }).Get(), &token);
                            browser->add_HistoryChanged(
                                Callback<ICoreWebView2HistoryChangedEventHandler>(
                                    [](ICoreWebView2*, IUnknown*) -> HRESULT {
                                        RefreshButtons();
                                        return S_OK;
                                    }).Get(), &token);
                            browser->add_NavigationStarting(
                                Callback<ICoreWebView2NavigationStartingEventHandler>(
                                    [](ICoreWebView2*, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT {
                                        LPWSTR uri = nullptr;
                                        bool valid = SUCCEEDED(args->get_Uri(&uri)) && uri && Allowed(uri);
                                        if (uri) CoTaskMemFree(uri);
                                        if (!valid) args->put_Cancel(TRUE);
                                        return S_OK;
                                    }).Get(), &token);
                            browser->add_PermissionRequested(
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
                            browser->add_NewWindowRequested(
                                Callback<ICoreWebView2NewWindowRequestedEventHandler>(
                                    [](ICoreWebView2*, ICoreWebView2NewWindowRequestedEventArgs* args) -> HRESULT {
                                        args->put_Handled(TRUE);
                                        LPWSTR uri = nullptr;
                                        if (SUCCEEDED(args->get_Uri(&uri)) && uri) {
                                            if (browser && Allowed(uri)) browser->Navigate(uri);
                                            CoTaskMemFree(uri);
                                        }
                                        return S_OK;
                                    }).Get(), &token);
                            controller->add_AcceleratorKeyPressed(
                                Callback<ICoreWebView2AcceleratorKeyPressedEventHandler>(
                                    [](ICoreWebView2Controller*, ICoreWebView2AcceleratorKeyPressedEventArgs* args) -> HRESULT {
                                        COREWEBVIEW2_KEY_EVENT_KIND eventKind{};
                                        UINT key = 0;
                                        args->get_KeyEventKind(&eventKind);
                                        args->get_VirtualKey(&key);
                                        if (eventKind != COREWEBVIEW2_KEY_EVENT_KIND_KEY_DOWN &&
                                            eventKind != COREWEBVIEW2_KEY_EVENT_KIND_SYSTEM_KEY_DOWN) return S_OK;
                                        bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
                                        bool alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
                                        if (ctrl && key == 'L') {
                                            SetFocus(address);
                                            SendMessageW(address, EM_SETSEL, 0, -1);
                                            args->put_Handled(TRUE);
                                        } else if (key == VK_F5 || (ctrl && key == 'R')) {
                                            if (browser) browser->Reload();
                                            args->put_Handled(TRUE);
                                        } else if (alt && key == VK_LEFT) {
                                            if (browser) browser->GoBack();
                                            args->put_Handled(TRUE);
                                        } else if (alt && key == VK_RIGHT) {
                                            if (browser) browser->GoForward();
                                            args->put_Handled(TRUE);
                                        }
                                        return S_OK;
                                    }).Get(), &token);
                            Layout();
                            RefreshButtons();
                            browser->Navigate(L"about:blank");
                            SetFocus(address);
                            return S_OK;
                        }).Get());
                if (FAILED(requested)) Error(L"controller request", requested);
                return S_OK;
            }).Get());
    if (FAILED(hr)) Error(L"environment request", hr);
}
LRESULT CALLBACK AddressProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR, DWORD_PTR) {
    if (msg == WM_KEYDOWN && wp == VK_RETURN) {
        Navigate();
        return 0;
    }
    if (msg == WM_KEYDOWN && wp == VK_ESCAPE) {
        if (browser) {
            LPWSTR current = nullptr;
            if (SUCCEEDED(browser->get_Source(&current)) && current) {
                SetWindowTextW(hwnd, _wcsicmp(current, L"about:blank") == 0 ? L"" : current);
                CoTaskMemFree(current);
            }
        }
        if (controller) controller->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
        return 0;
    }
    return DefSubclassProc(hwnd, msg, wp, lp);
}
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        window = hwnd;
        DWORD buttonStyle = WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON;
        back = CreateWindowW(L"BUTTON", L"<", buttonStyle, 0, 0, 0, 0, hwnd,
                             reinterpret_cast<HMENU>(BACK), nullptr, nullptr);
        forward = CreateWindowW(L"BUTTON", L">", buttonStyle, 0, 0, 0, 0, hwnd,
                                reinterpret_cast<HMENU>(FORWARD), nullptr, nullptr);
        reload = CreateWindowW(L"BUTTON", L"R", buttonStyle, 0, 0, 0, 0, hwnd,
                               reinterpret_cast<HMENU>(RELOAD), nullptr, nullptr);
        address = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                  WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                  0, 0, 0, 0, hwnd, reinterpret_cast<HMENU>(ADDRESS), nullptr, nullptr);
        go = CreateWindowW(L"BUTTON", L"Go", buttonStyle, 0, 0, 0, 0, hwnd,
                           reinterpret_cast<HMENU>(GO), nullptr, nullptr);
        HFONT font = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        for (HWND child : {back, forward, reload, address, go})
            SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        SetWindowSubclass(address, AddressProc, 1, 0);
        RefreshButtons();
        return 0;
    }
    case WM_SIZE:
        Layout();
        return 0;
    case WM_COMMAND:
        if (HIWORD(wp) == BN_CLICKED && browser) {
            switch (LOWORD(wp)) {
            case BACK: browser->GoBack(); break;
            case FORWARD: browser->GoForward(); break;
            case RELOAD: browser->Reload(); break;
            case GO: Navigate(); break;
            }
        }
        return 0;
    case WM_GETMINMAXINFO: {
        auto* minMax = reinterpret_cast<MINMAXINFO*>(lp);
        minMax->ptMinTrackSize.x = 450;
        minMax->ptMinTrackSize.y = 250;
        return 0;
    }
    case WM_DESTROY:
        if (controller) controller->Close();
        browser.Reset();
        controller.Reset();
        window = nullptr;
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(hwnd, msg, wp, lp);
    }
}
}  // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    SetProcessDPIAware();
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        MessageBoxW(nullptr, L"COM initialization failed.", kApp, MB_OK | MB_ICONERROR);
        return 1;
    }
    INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&controls);
    WNDCLASSW cls{};
    cls.hInstance = instance;
    cls.lpszClassName = kClass;
    cls.lpfnWndProc = WndProc;
    cls.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    cls.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    if (!RegisterClassW(&cls)) {
        CoUninitialize();
        return 1;
    }
    HWND hwnd = CreateWindowExW(0, kClass, kApp, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                                CW_USEDEFAULT, 1100, 750, nullptr, nullptr, instance, nullptr);
    if (!hwnd) {
        CoUninitialize();
        return 1;
    }
    ShowWindow(hwnd, show);
    UpdateWindow(hwnd);
    InitBrowser();
    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    CoUninitialize();
    return static_cast<int>(msg.wParam);
}
