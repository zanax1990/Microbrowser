# Testing Microbrowser

This checklist distinguishes **automated build checks** from **manual browser behavior**. A successful compile is not a functional, performance, privacy, or security certification.

## Automated checks on GitHub

- **Build Windows Browser:** compiles the Windows x64 executable on a GitHub-hosted Windows runner, verifies its presence, prints its SHA-256 hash, and uploads it as a downloadable artifact.
- **Security Check (Windows Defender):** independently rebuilds the EXE and attempts an on-demand scan with the Defender engine available on the runner. Runner protection settings and signature version are logged.

Workflows run on pushes and pull requests targeting `main`, and can be triggered manually. Defender on-demand scanning is not a substitute for an external security review.

## Manual Windows checks

Use a clean Windows 10/11 x64 machine or a VM with WebView2 Runtime installed.

- [ ] Open `Microbrowser.exe`; a single blank **tab** appears.
- [ ] Create three tabs with **+** or **Ctrl+T** and navigate each to a distinct website.
- [ ] Switch tabs with the tab strip, **Ctrl+Tab**, and **Ctrl+Shift+Tab**; confirm each tab keeps its own URL and navigation state.
- [ ] Close tabs with **x** or **Ctrl+W**; closing the last tab should produce a new blank tab.
- [ ] Use **Private** or **Ctrl+Shift+N**; verify a separate titled window opens, does not share ordinary session cookies, and fails closed if private APIs are unavailable.
- [ ] After closing all private windows, restart a new private window and check that private session cookies/history are gone (without expecting downloads or OS traces to disappear).
- [ ] Download a small known file over HTTPS; verify **Downloads** / **Ctrl+J** shows filename, progress and completion.
- [ ] Cancel a second download and verify it stops; verify **Open folder** opens the correct location.
- [ ] With multiple tabs, toggle **Eco: ON**; confirm background pages remain navigable after being reactivated, and compare process-tree RAM/CPU against Eco: OFF.
- [ ] Navigate to `https://example.com`; verify that the page renders.
- [ ] Search for a non-sensitive phrase in the address bar; DuckDuckGo results load.
- [ ] Check Back, Forward, Reload, and Go buttons.
- [ ] Check **Ctrl+L**, **F5**, **Ctrl+R**, **Alt+Left**, and **Alt+Right**.
- [ ] Open an HTTP and HTTPS link, including a link requesting a new window.
- [ ] Test a broken URL, an unreachable site, and an invalid address.
- [ ] Verify expected handling of camera, microphone, location, and notification requests.
- [ ] Close and reopen; record whether the WebView2 profile retained expected cookies/history.
- [ ] Check whether Windows, Chrome, or SmartScreen generates a security warning; record the exact wording.
- [ ] Check that the app closes without leaving its main window or unintended processes open.

## Measure memory fairly

Executable size is **not** RAM usage. WebView2 can launch multiple subprocesses that must be counted.

1. Reboot or use equivalent clean profiles.
2. Open the **same page** in Microbrowser and in Chrome/Edge under comparable conditions.
3. Use Windows Task Manager or Process Explorer to measure total memory used by each browser's process tree (including all WebView2 subprocesses).
4. Record idle and loaded-page measurements, CPU use, runtime versions, page URL, and repeat counts.
5. Compare multiple runs; do not advertise a memory advantage until independently reproduced.

## Known limitations

- No custom ad/tracker blocker, bookmarks, or permanent download history. Private mode is not anonymous browsing; downloaded files persist.
- Eco mode uses WebView2's best-effort TrySuspend API; savings are not guaranteed, particularly for audio/video or active pages.
- No independent security or privacy audit.
- Some websites, login flows, pop-ups, or browser integrations may fail.
- Unsigned binary downloads may trigger reputation-based security warnings.
- Tests in GitHub Actions do **not** launch the GUI to verify site rendering.
