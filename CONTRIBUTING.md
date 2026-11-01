# Contributing

Microbrowser is a small Windows browser shell. Contributions that keep the codebase understandable and the application minimal are welcome.

## Before opening a pull request

1. Check [existing issues](https://github.com/zanax1990/Microbrowser/issues) for similar reports.
2. Describe the change and why it is useful for a minimal browser.
3. Keep code, identifiers, comments, and documentation in English.
4. Run the Windows x64 build when possible, or use the [GitHub Actions build](https://github.com/zanax1990/Microbrowser/actions/workflows/build-windows.yml).
5. Test the relevant items in [TESTING.md](TESTING.md), and document anything you could not test.

Avoid committing `_deps/`, `dist/`, binary SDK packages, browser profiles, tokens, or personal browsing data.

## Bugs

When filing a non-security bug, include:
- Windows version and system architecture
- WebView2 Runtime version, if known
- Steps to reproduce and expected/actual behavior
- Relevant error messages (remove private URLs, cookies, credentials, and tokens)
- Whether the issue also happens in Microsoft Edge

**Security vulnerabilities:** follow [SECURITY.md](SECURITY.md) instead of publishing exploit details.

## Licensing

By contributing to this MIT-licensed repository, you agree that your contribution may be distributed under the project's [MIT License](LICENSE). Do not submit code or assets you do not have permission to contribute. Microsoft's WebView2 components retain their own terms.
