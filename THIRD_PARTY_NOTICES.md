# Third-party components and licenses

Microbrowser's original application source is licensed under the [MIT License](LICENSE).

## Microsoft Edge WebView2

Microbrowser uses Microsoft's WebView2 APIs and requires the **Microsoft Edge WebView2 Runtime** installed on Windows.

The build downloads the Microsoft.Web.WebView2 SDK from NuGet and statically links its WebView2LoaderStatic library into the generated executable. Microsoft components are **not** covered by Microbrowser's MIT License. Their use and redistribution are governed by Microsoft's applicable license terms.

- [Microsoft.Web.WebView2 NuGet package](https://www.nuget.org/packages/Microsoft.Web.WebView2/)
- [WebView2 distribution guidance](https://learn.microsoft.com/en-us/microsoft-edge/webview2/concepts/distribution)
- [WebView2 SDK license information](https://www.nuget.org/packages/Microsoft.Web.WebView2/#license-info)

The repository does not commit the SDK package or the WebView2 Runtime. Build scripts fetch the SDK when compiling. Before distributing binaries, review the Microsoft terms applicable to the SDK/loader and Runtime.

## GitHub Actions dependencies

The GitHub Actions workflows use third-party actions for checkout, MSVC setup and artifact upload. Those actions remain subject to their own respective licenses and are not part of the Microbrowser source-code license.

No affiliation with or endorsement by Microsoft is implied.
