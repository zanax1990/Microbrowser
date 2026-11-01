# Microbrowser

[![Windows build](https://github.com/zanax1990/Microbrowser/actions/workflows/build-windows.yml/badge.svg)](https://github.com/zanax1990/Microbrowser/actions/workflows/build-windows.yml)
[![MIT License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

A minimal web browser for **Windows 10/11 (x64)**, written in native C++ using the Win32 API and **Microsoft Edge WebView2**.

Microbrowser is a small *browser application*, **not a new browser engine**. It uses Microsoft's installed WebView2 Runtime (Chromium-based) to render websites.

> **Status: early prototype.** The Windows build has succeeded in GitHub Actions, and a basic page has been opened on Windows. Comprehensive compatibility, security, and memory benchmarks have not been completed.

## Features

- One window, one web page; no tabs or extensions
- Address bar with HTTP/HTTPS navigation
- Search phrases sent to DuckDuckGo
- Back, Forward, Reload, and Go controls
- Basic keyboard shortcuts
- Local browser profile stored on your computer
- No application-specific analytics, accounts, sync, or advertising

**Not included:** ad blocking, private/incognito mode, a full download manager UI, bookmarks, extension support, or a separate rendering engine. Websites themselves may show advertisements or collect data.

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

The app opens with a blank page. Searches use DuckDuckGo. The browser shell does not promise that every website or web feature will work.

## Privacy and security

- The application source contains no dedicated analytics, ad injection, sync, or custom tracking endpoints.
- Browsing history, cookies, and other WebView2 profile data can persist under `%LOCALAPPDATA%\Microbrowser\Profile`.
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
