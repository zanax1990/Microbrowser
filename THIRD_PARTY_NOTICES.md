# Third-party components and licenses

The **original Microbrowser application source code** is distributed under the repository's [MIT License](LICENSE). That license does not grant rights to Microsoft's separately licensed components.

## Microsoft Edge WebView2

Microbrowser uses the **Microsoft Edge WebView2 Runtime** to render web content. The Runtime must be installed separately on the Windows device. This application does not bundle its own browser rendering engine.

The build downloads the **Microsoft.Web.WebView2 SDK 1.0.4314.35** from Microsoft's NuGet package and links `WebView2LoaderStatic.lib` into the Windows executable. The embedded Microsoft loader and Microsoft's Runtime are governed by **Microsoft's applicable terms**, not by the Microbrowser MIT License.

Official sources:
- [Microsoft.Web.WebView2 SDK package and license](https://www.nuget.org/packages/Microsoft.Web.WebView2/1.0.4314.35)
- [Microsoft: distribute your WebView2 app and Runtime](https://learn.microsoft.com/en-us/microsoft-edge/webview2/concepts/distribution)
- [Microsoft: Evergreen vs. fixed-version Runtime](https://learn.microsoft.com/en-us/microsoft-edge/webview2/concepts/evergreen-vs-fixed-version)
- [Official WebView2 Runtime download](https://developer.microsoft.com/en-us/microsoft-edge/webview2/)

The repository does not check in the Microsoft SDK or Runtime. The static loader is **included in compiled builds**, so review Microsoft's terms before redistributing those executables. Microbrowser does not imply Microsoft affiliation or endorsement.

## GitHub Actions

The workflows use third-party actions such as `actions/checkout`, `actions/upload-artifact`, and `ilammy/msvc-dev-cmd`. Those projects have their own licenses and maintenance policies. Their source and licenses can be reviewed at their respective GitHub repositories.
