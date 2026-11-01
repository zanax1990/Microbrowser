# Privacy

This document describes the current Microbrowser source code. It is not a statement about every network request made by WebView2, Windows, or websites you visit.

## Data handled by Microbrowser

Microbrowser does not implement its own account system, analytics service, cloud synchronization, crash-report upload, advertising, or telemetry endpoint.

The application uses the installed Microsoft Edge WebView2 Runtime to load web pages. Browser data such as cookies, local storage, cache, and history may be saved locally in:

```text
%LOCALAPPDATA%\Microbrowser\Profile
```

This browser profile can persist between launches. There is currently no in-app "clear browsing data" control or private browsing mode. To clear the local profile, close Microbrowser completely and remove this folder in Windows. Doing so may sign you out of websites and erase stored site data.

## Network requests

- Web pages can communicate with their own servers and third parties, including trackers and advertising services.
- If you type a search phrase instead of a URL, the phrase is sent to **DuckDuckGo**.
- The WebView2 Runtime and Windows can communicate with Microsoft services under their own product settings and policies.
- Microbrowser does **not** currently block cookies, ads, fingerprinting, or trackers.

The application denies site permission requests for the camera, microphone, geolocation, and notifications. This is **not** a complete browser privacy or security model.

## What this does not guarantee

No custom analytics does **not** mean zero data collection by websites, the WebView2 Runtime, Windows, or network providers. The project has not received an independent privacy audit. Review the code and the relevant third-party policies before using it with sensitive information.

Related: [SECURITY.md](SECURITY.md) and [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
