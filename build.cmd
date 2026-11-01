@echo off
setlocal
cd /d "%~dp0"
where cl.exe >nul 2>nul
if errorlevel 1 (
  echo ERROR: Microsoft C++ compiler not found.
  echo Run this from "x64 Native Tools Command Prompt for VS 2022".
  exit /b 1
)
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\get-sdk.ps1"
if errorlevel 1 exit /b 1
if not exist dist mkdir dist
set "SDK=%~dp0_deps\webview2\build\native"
if not exist "%SDK%\x64\WebView2LoaderStatic.lib" (
  echo ERROR: WebView2LoaderStatic.lib is missing.
  exit /b 1
)
cl.exe /nologo /std:c++17 /EHsc /O1 /GL /Gy /MT /W4 /DWIN32 /DUNICODE /D_UNICODE /D_WIN32_WINNT=0x0A00 /I"%SDK%\include" /Fo"%~dp0dist\Microbrowser.obj" /Fe"%~dp0dist\Microbrowser.exe" main.cpp /link /SUBSYSTEM:WINDOWS /LTCG /OPT:REF /OPT:ICF "%SDK%\x64\WebView2LoaderStatic.lib" user32.lib gdi32.lib ole32.lib shell32.lib comctl32.lib uuid.lib
if errorlevel 1 (
  echo BUILD FAILED.
  exit /b 1
)
echo BUILD SUCCESS: %~dp0dist\Microbrowser.exe
