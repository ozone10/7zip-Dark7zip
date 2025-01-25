@echo off

cd /d "%~dp0"

for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
  set InstallDir=%%i
)

setlocal

if "%1" == "" set ARCH=x64
if "%1" == "x64" set ARCH=x64
if "%1" == "x86" set ARCH=x64_x86
if "%1" == "arm64" set ARCH=x64_arm64

if exist "%InstallDir%\VC\Auxiliary\Build\vcvarsall.bat" (
  call "%InstallDir%\VC\Auxiliary\Build\vcvarsall.bat" %ARCH%

  rem pushd CPP\7zip
  rem nmake
  rem popd

  pushd CPP\7zip\UI\FileManager
  nmake
  popd

  pushd CPP\7zip\UI\GUI
  nmake
  popd

  pushd CPP\7zip\Bundles\SFXWin
  nmake
  popd

  if not exist "%ARCH%-bin\" mkdir "%ARCH%-bin"
  copy "CPP\7zip\Bundles\SFXWin\%ARCH%\7z.sfx" "%ARCH%-bin"
  copy "CPP\7zip\UI\FileManager\%ARCH%\7zFM.exe" "%ARCH%-bin"
  copy "CPP\7zip\UI\GUI\%ARCH%\7zG.exe" "%ARCH%-bin"
  copy "DarkMode\7zDark.ini" "%ARCH%-bin"
)

endlocal
