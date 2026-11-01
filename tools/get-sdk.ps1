$ErrorActionPreference = 'Stop'
$version = '1.0.4314.35'
$root = Split-Path -Parent $PSScriptRoot
$deps = Join-Path $root '_deps'
$package = Join-Path $deps 'webview2-sdk.zip'
$unpacked = Join-Path $deps 'webview2'
$header = Join-Path $unpacked 'build\native\include\WebView2.h'
if (Test-Path $header) {
    Write-Host "WebView2 SDK $version already available."
    exit 0
}
New-Item -ItemType Directory -Path $deps -Force | Out-Null
$uri = "https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/$version"
Write-Host "Downloading official WebView2 SDK $version from NuGet..."
Invoke-WebRequest -Uri $uri -OutFile $package -MaximumRedirection 10 -UseBasicParsing
if (Test-Path $unpacked) { Remove-Item $unpacked -Recurse -Force }
Expand-Archive -LiteralPath $package -DestinationPath $unpacked -Force
if (!(Test-Path $header)) { throw 'WebView2 SDK header missing from the downloaded NuGet package.' }
Write-Host 'SDK ready.'
