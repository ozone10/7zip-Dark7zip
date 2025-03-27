@echo off

cd /d "%~dp0"

for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
  set InstallDir=%%i
)

if not exist "%InstallDir%\VC\Auxiliary\Build\vcvarsall.bat" (
  echo vcvarsall.bat not found. Exiting.
  exit /b 1
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

call "%InstallDir%\VC\Auxiliary\Build\vcvarsall.bat" %ARCH%

if exist "CPP\7zip\UI\FileManager\%PLATFORM%\resource.res" (
  del /F /Q "CPP\7zip\UI\FileManager\%PLATFORM%\resource.res"
)

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

if not exist "%PLATFORM%-bin\" mkdir "%PLATFORM%-bin"
copy "CPP\7zip\Bundles\SFXWin\%PLATFORM%\7z.sfx" "%PLATFORM%-bin"
copy "CPP\7zip\UI\FileManager\%PLATFORM%\7zFM.exe" "%PLATFORM%-bin"
copy "CPP\7zip\UI\GUI\%PLATFORM%\7zG.exe" "%PLATFORM%-bin"
copy "DarkMode\7zDark.ini" "%PLATFORM%-bin"

echo Building fluent version
if not exist "tmp\" mkdir "tmp"
move "CPP\7zip\UI\FileManager\*.bmp" "tmp" >nul
xcopy "DarkMode\icons\*.bmp" "CPP\7zip\UI\FileManager" /Y >nul
del /F /Q "CPP\7zip\UI\FileManager\%PLATFORM%\resource.res"

pushd CPP\7zip\UI\FileManager
nmake
popd

if not exist "%PLATFORM%-fluent-bin\" mkdir "%PLATFORM%-fluent-bin"
copy "CPP\7zip\Bundles\SFXWin\%PLATFORM%\7z.sfx" "%PLATFORM%-fluent-bin"
copy "CPP\7zip\UI\FileManager\%PLATFORM%\7zFM.exe" "%PLATFORM%-fluent-bin"
copy "CPP\7zip\UI\GUI\%PLATFORM%\7zG.exe" "%PLATFORM%-fluent-bin"
copy "DarkMode\7zDark.ini" "%PLATFORM%-fluent-bin"

xcopy "tmp\*.bmp" "CPP\7zip\UI\FileManager" /Y >nul
rmdir /S /Q "tmp"

rem pause

endlocal
