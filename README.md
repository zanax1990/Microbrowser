# Microbrowser

[![Windows build](https://github.com/zanax1990/Microbrowser/actions/workflows/build-windows.yml/badge.svg)](https://github.com/zanax1990/Microbrowser/actions/workflows/build-windows.yml)
[![MIT License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

A minimal web browser for **Windows 10/11 (x64)**, written in native C++ using the Win32 API and **Microsoft Edge WebView2**.

Microbrowser is a small *browser application*, **not a new browser engine**. It uses Microsoft's installed WebView2 Runtime (Chromium-based) to render websites.

> **Status: early prototype.** The Windows build has succeeded in GitHub Actions, and a basic page has been opened on Windows. Comprehensive compatibility, security, and memory benchmarks have not been completed.

## Features

- Tabs: up to 32 open pages, with + / x buttons and keyboard shortcuts
- Separate **Private** window using WebView2's built-in InPrivate controller/profile mode
- **Eco: ON/OFF**: hide inactive tabs and request WebView2 to suspend them when Eco is enabled
- Downloads panel with filename, progress, cancel, and open-folder controls
- Address bar with HTTP/HTTPS navigation and DuckDuckGo search
- Back, Forward, Reload, and Go controls
- Local browser profile for ordinary windows
- No application-specific analytics, accounts, sync, or advertising

**Not included:** ad blocking, bookmarks, extensions, full-featured download history, or a separate rendering engine. WebView2 handles the actual rendering and downloads.

**Memory note:** Eco mode asks WebView2 to suspend hidden pages. Suspension is best-effort, may not work for some sites, and does not guarantee lower total RAM usage.

## Download for Windows

You do **not** need Visual Studio to run the program.

1. Open **[Build Windows Browser — GitHub Actions](https://github.com/zanax1990/Microbrowser/actions/workflows/build-windows.yml)**.
2. Select a successful run (green check mark).
3. Under **Artifacts**, download **Microbrowser-Windows-x64**. You may need to sign in to GitHub.
4. Extract the ZIP file, then run `Microbrowser.exe`.

The build artifact expires **14 days** after the run. There is no installer or signed release package at present. If the artifact is missing, use **Run workflow** to produce a new one.

**Windows requirement:** the [Microsoft Edge WebView2 Runtime](https://developer.microsoft.com/en-us/microsoft-edge/webview2/) must be installed. Most Windows 11 devices and many Windows 10 devices already have it. If Microbrowser reports that WebView2 cannot start, install the official **Evergreen Runtime** from Microsoft.

### Browser or Windows security warnings

This is an unsigned prototype. Chrome, Microsoft Defender SmartScreen, or other security software may flag an unfamiliar executable or ZIP. A warning is **not proof of malware**, but it is also **not proof the file is safe**.

Do not disable browser/antivirus protection just to install it. You can inspect this repository, verify the executable's SHA-256 hash against its GitHub Actions build log, and/or compile from source. An automated antivirus scan does not guarantee safety. See [SECURITY.md](SECURITY.md).

## Build from source

For a local Windows x64 build:

1. Install **Visual Studio 2022 Build Tools** with **Desktop development with C++**, including the Windows SDK, or use Visual Studio with the same components.
2. Open **x64 Native Tools Command Prompt for VS 2022**.
3. Clone the repository and run:

```cmd
git clone https://github.com/zanax1990/Microbrowser.git
cd Microbrowser
build.cmd
```

Output: `dist\Microbrowser.exe`.

The build script downloads the official **Microsoft.Web.WebView2 SDK 1.0.4314.35** from NuGet. The loader is statically linked; the WebView2 Runtime remains a separate Windows prerequisite. You can also build entirely online using the GitHub Actions workflow above.

## Usage

| Action | Control |
| --- | --- |
| Open a website or search | Type into the address bar and press **Enter** or **Go** |
| Focus the address bar | **Ctrl+L** |
| Reload | **F5** or **Ctrl+R** |
| Previous/next page | **Alt+Left** / **Alt+Right** or the toolbar buttons |
| New/close tab | **Ctrl+T** / **Ctrl+W**, or **+** / **x** |
| Next/previous tab | **Ctrl+Tab** / **Ctrl+Shift+Tab** |
| Private window | **Ctrl+Shift+N** or **Private** |
| Downloads panel | **Ctrl+J** or **Downloads** |
| Eco mode | **Eco: ON / OFF** button |

The app opens with a blank tab. New windows requested by websites may be opened as tabs, but complex popup/login flows are not guaranteed to work.

### Private windows

The **Private** button launches a separate Microbrowser process using WebView2's **InPrivate** mode. It is designed not to persist private browsing history, cookies, or local storage to the regular browsing profile. It does **not** provide anonymity, prevent network tracking, or remove downloaded files from disk. If the installed WebView2 Runtime does not support InPrivate controller options, the feature **fails closed** rather than silently using normal mode.

### Downloads

Open the **Downloads** panel to see progress, cancel a selected transfer, or open its destination folder. Downloads use WebView2's default destination. The app does not include pause/resume controls or a permanent download history; the list is kept only during the current process session.

## Privacy and security

- The application source contains no dedicated analytics, ad injection, sync, or custom tracking endpoints.
- Normal-mode browsing history, cookies, and other WebView2 profile data can persist under `%LOCALAPPDATA%\Microbrowser\Profile`.
- Private windows use WebView2 InPrivate mode, but downloaded files, network logs, OS records, and remote service records may still persist.
- WebView2, the OS, search providers, and visited websites can make network requests independently of the application.
- Camera, microphone, geolocation, and notification permission requests are denied by the app.
- There is no independent security audit. **Do not assume the app is safer, more private, or lighter in RAM than Chrome.**

See [PRIVACY.md](PRIVACY.md), [SECURITY.md](SECURITY.md), and [TESTING.md](TESTING.md).

## Project files

- `main.cpp` — native Win32 interface and WebView2 integration
- `build.cmd` — x64 MSVC build
- `tools/get-sdk.ps1` — pinned SDK downloader
- `.github/workflows/` — Windows build and Defender scan
- `TESTING.md` — repeatable test checklist

Bug reports and contributions are welcome: [CONTRIBUTING.md](CONTRIBUTING.md).

## License

The original Microbrowser source code is available under the [MIT License](LICENSE), copyright © 2026 zanax1990.

**Microsoft's WebView2 SDK, statically linked loader, and Runtime have separate Microsoft license terms.** Microbrowser does not own or relicense those components. Read [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) before redistributing compiled executables.

Microbrowser is an independent project and is not affiliated with Microsoft.
