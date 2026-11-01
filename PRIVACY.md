# Privacy

This document describes the current Microbrowser source code. It is not a statement about every network request made by WebView2, Windows, or websites you visit.

## Data handled by Microbrowser

Microbrowser does not implement its own account system, analytics service, cloud synchronization, crash-report upload, advertising, or telemetry endpoint.

The application uses the installed Microsoft Edge WebView2 Runtime to load web pages. Browser data such as cookies, local storage, cache, and history may be saved locally in:

```text
%LOCALAPPDATA%\Microbrowser\Profile
```

This browser profile can persist between launches. There is currently no in-app "clear browsing data" control. To clear the normal-mode local profile, close Microbrowser and related WebView2 processes, then remove this folder in Windows. Doing so may sign you out of websites and erase stored site data.

## Private window mode

The **Private** button (or **Ctrl+Shift+N**) starts a separate Microbrowser window/process using WebView2's built-in **InPrivate** controller options. WebView2 uses an off-the-record profile for that window: private history, cookies, and local storage are not intended to persist after the private session ends. The application does not implement any custom persistent private-history log.

This does **not** guarantee that no traces exist. Downloads remain on disk; visited sites, ISPs, administrators, Windows, and Microsoft WebView2 may still retain information under their own policies. A private window is not a VPN, network anonymity tool, or guarantee of zero disk writes.

If InPrivate cannot be initialized, Microbrowser displays an error rather than falling back to a normal browsing controller.

## Downloads

Downloads are handled by WebView2 using its default destination path. The downloads panel displays per-session progress and permits canceling a transfer; it does not keep its own permanent download list. Files downloaded while using Private mode remain on disk unless deleted by the user.

## Network requests

- Web pages can communicate with their own servers and third parties, including trackers and advertising services.
- If you type a search phrase instead of a URL, the phrase is sent to **DuckDuckGo**.
- The WebView2 Runtime and Windows can communicate with Microsoft services under their own product settings and policies.
- Microbrowser does **not** currently block cookies, ads, fingerprinting, or trackers.

The application denies site permission requests for the camera, microphone, geolocation, and notifications. This is **not** a complete browser privacy or security model.

## What this does not guarantee

No custom analytics does **not** mean zero data collection by websites, the WebView2 Runtime, Windows, or network providers. The project has not received an independent privacy audit. Review the code and the relevant third-party policies before using it with sensitive information.

Related: [SECURITY.md](SECURITY.md) and [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
