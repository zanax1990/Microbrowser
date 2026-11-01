# Security Policy

## Project maturity

Microbrowser is an **early prototype**, not a security-hardened browser. It wraps Microsoft Edge WebView2 and does not implement its own comprehensive protection against malicious websites, downloads, phishing, tracking, or unsafe navigation.

The project has **not** undergone an independent security audit. Do not use it as a substitute for a fully supported browser when security or compatibility is critical.

## Updates and dependencies

- Keep **Windows** and the **Microsoft Edge WebView2 Runtime** updated.
- The SDK version used at build time is pinned in `tools/get-sdk.ps1`; updating the installed WebView2 Runtime is a separate process.
- The compiled EXE is currently **unsigned**. Security warnings should be investigated, not automatically bypassed.
- GitHub Actions compiles the Windows executable and provides a separate Windows Defender on-demand scan. A passing build or scan is **not** a malware-free or security-certified verdict.
- The application retains local browser profile data by default; see [PRIVACY.md](PRIVACY.md).

## Reporting a vulnerability

Please **do not post exploit details, credentials, or sensitive data in a public GitHub issue**.

Use GitHub's **private vulnerability reporting** for this repository if it is available. If not, open a minimal public issue requesting a private contact channel, without vulnerability details. The repository owner can then arrange a private report.

Ordinary reproducible bugs that do not disclose security-sensitive details can be reported through [GitHub Issues](https://github.com/zanax1990/Microbrowser/issues).

There is no promised response time or formal support agreement.
