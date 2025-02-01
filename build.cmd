@echo off

cd /d "%~dp0"

for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
  set InstallDir=%%i
)

setlocal

set PLATFORM=%1

if "%PLATFORM%" == "" (
  set ARCH=x64
  set PLATFORM=x64
)
if "%PLATFORM%" == "x64" set ARCH=x64
if "%PLATFORM%" == "x86" set ARCH=x64_x86
if "%PLATFORM%" == "arm64" set ARCH=x64_arm64

if exist "%InstallDir%\VC\Auxiliary\Build\vcvarsall.bat" (
  call "%InstallDir%\VC\Auxiliary\Build\vcvarsall.bat" %ARCH%

  pushd CPP\7zip\UI\FileManager
  nmake
  popd

  pushd CPP\7zip\UI\GUI
  nmake
  popd

  pushd CPP\7zip\Bundles\SFXWin
  nmake
  popd

  if not exist "%PLATFORM%-bin\" mkdir "%PLATFORM%-bin"
  copy "CPP\7zip\Bundles\SFXWin\%PLATFORM%\7z.sfx" "%PLATFORM%-bin"
  copy "CPP\7zip\UI\FileManager\%PLATFORM%\7zFM.exe" "%PLATFORM%-bin"
  copy "CPP\7zip\UI\GUI\%PLATFORM%\7zG.exe" "%PLATFORM%-bin"
  copy "DarkMode\7zRes\7zDark.ini" "%PLATFORM%-bin"
  copy "COPYING" "%PLATFORM%-bin"
  copy "LICENSE.md" "%PLATFORM%-bin"
)

endlocal
