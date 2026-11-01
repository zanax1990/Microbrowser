# Microbrowser

A minimal, single-window Windows browser built with native C++ (Win32) and Microsoft WebView2.

## Goals
- Minimal interface: address bar, Back, Forward, Reload, Go.
- No tabs, extensions, custom sync, advertisements, or application telemetry.
- No bundled browser engine: uses the installed Microsoft Edge WebView2 Runtime.
- Windows 10/11 x64 only.

**Note:** The tiny executable does not guarantee low total memory use. WebView2 still runs browser subprocesses. Microsoft and visited sites may collect telemetry; this project does not promise zero tracking.

## Download an executable without installing a compiler
1. Open **Actions** in this repository.
2. Select **Build Windows Browser**, and open the latest successful run.
3. Download artifact **Microbrowser-Windows-x64** and extract its ZIP.
4. Launch **Microbrowser.exe** on Windows.

The build downloads the official Microsoft WebView2 SDK and compiles on a GitHub-hosted Windows runner. No local Visual Studio installation is needed.

## Run
- Type a web address or search phrase into the address bar.
- Enter: navigate; Ctrl+L: address bar; F5 or Ctrl+R: reload; Alt+Left/Right: history.

## Privacy
- App itself has no analytics, tracking endpoints, accounts or background update checks.
- The built-in browser engine still makes network requests and may have independent telemetry.
- Browser profile is stored at `%LOCALAPPDATA%\Microbrowser\Profile`; it persists until manually removed.
- Device permissions (camera, microphone, geolocation, notifications) are denied.

## Status
Building on Windows is automated by GitHub Actions. The existence of an EXE does not prove every website works or that RAM use is lower than Chrome.
