# Microbrowser testing

## Automated (GitHub Actions)
- Build on Windows Server 2022 with MSVC x64.
- Confirm dist/Microbrowser.exe exists.
- Compute SHA256 and upload executable as an artifact.

## Manual tests required on Windows
1. Open Microbrowser.exe and confirm an empty browser window opens.
2. Enter https://example.com and verify the page is rendered.
3. Enter a query ("light browser test") and verify DuckDuckGo results.
4. Use Back, Forward, Reload, Go and Ctrl+L shortcuts.
5. Confirm an https link opens in the same window.
6. Confirm camera, microphone, location and notification permissions are denied.
7. Restart and confirm whether history/cookies persist.
8. Compare total memory usage of all WebView2 subprocesses with Chrome under the same pages; do not compare EXE size alone.

## Known limitations
- Some sites require browser functionality this small shell does not provide.
- Uses Microsoft WebView2 Runtime on the local machine, so the program is not a separate web engine.
- Does not implement a hardened URL/navigation safety policy or privacy auditing.
- A successful compile is not a functional or security validation.
