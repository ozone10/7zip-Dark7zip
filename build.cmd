@echo off

cd /d "%~dp0"

setlocal EnableDelayedExpansion

set VERSION=2604
set VERSION_TAG=26.04

rem Platform
set PLATFORM=%1
rem set URL=https://www.7-zip.org/a/7z%VERSION%-%PLATFORM%.exe
set URL=https://github.com/ip7z/7zip/releases/download/%VERSION_TAG%/7z%VERSION%-%PLATFORM%.exe

if "%PLATFORM%"=="x64" (
  set ARCH=x64
) else if "%PLATFORM%"=="x86" (
  set ARCH=x64_x86
  set URL=https://github.com/ip7z/7zip/releases/download/%VERSION_TAG%/7z%VERSION%.exe
) else if "%PLATFORM%"=="arm64" (
  set ARCH=x64_arm64
) else (
  set ARCH=x64
  set PLATFORM=x64
  set URL=https://github.com/ip7z/7zip/releases/download/%VERSION_TAG%/7z%VERSION%-%PLATFORM%.exe
)

rem VS Detection
for /f "usebackq tokens=*" %%i in (`
  "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
`) do (
  set InstallDir=%%i
)

if not exist "%InstallDir%\VC\Auxiliary\Build\vcvarsall.bat" (
  echo Visual Studio vcvarsall.bat not found.
  exit /b 1
)

call "%InstallDir%\VC\Auxiliary\Build\vcvarsall.bat" %ARCH%

rem Paths
set FM=CPP\7zip\UI\FileManager
set GUI=CPP\7zip\UI\GUI
set SFX=CPP\7zip\Bundles\SFXWin
set FMT=CPP\7zip\Bundles\Format7zF
set CONS=CPP\7zip\UI\Console
set SFXC=CPP\7zip\Bundles\SFXCon
set EXP=CPP\7zip\UI\Explorer
set DARK=DarkMode\7zRes

rem Clean
if "%2"=="clean" (
  echo Cleaning output folders...

  for %%D in (
    "%PLATFORM%-bin"
    "%PLATFORM%-fluent-bin"
    "%PLATFORM%-fluent-installer-bin"
  ) do (
    if exist %%D rmdir /S /Q %%D
  )

  for %%D in (
    "%FM%\%PLATFORM%"
    "%GUI%\%PLATFORM%"
    "%SFX%\%PLATFORM%"
    "%FMT%\%PLATFORM%"
    "%CONS%\%PLATFORM%"
    "%SFXC%\%PLATFORM%"
    "%EXP%\%PLATFORM%"
  ) do (
    if exist %%D rmdir /S /Q %%D
  )

  if "%PLATFORM%"=="x64" (
    if exist "%EXP%\x86" rmdir /S /Q "%EXP%\x86"
  )

  echo Clean complete.
  exit /b
)

if not exist "DarkMode\lib\src\StdAfx.h" (
  echo DarkMode\lib\src\StdAfx.h not found. Did you get darkmodelib submodule?
  exit /b 1
)

set MODE=%2

if not "%MODE%"=="" if not "%MODE%"=="standard" if not "%MODE%"=="fluent" if not "%MODE%"=="installer" if not "%MODE%"=="full" if not "%MODE%"=="all" (
  echo Usage: %~nx0 [x64^|x86^|arm64] [standard^|fluent^|installer^|full^|all^|clean]
  exit /b 1
)

if not exist "tmp\" mkdir "tmp"
move "DarkMode\lib\src\StdAfx.h" "tmp" >nul
xcopy "%DARK%\StdAfx.h" "DarkMode\lib\src" /y >nul

rem Full mode runs only the full nmake build.
if "%MODE%"=="full" goto build_full

rem All mode runs the full nmake build, then the fluent variant.
if "%MODE%"=="all" goto build_all

rem Standard mode builds the standard FM, GUI, and SFX.
rem Blank mode builds standard, fluent, and installer outputs.
if "%MODE%"=="" goto build_standard
if "%MODE%"=="standard" goto build_standard

rem Fluent and installer modes build the fluent variant.
goto build_fluent_variant

:build_standard
call :del_fm
call :build "%FM%" "7zFM.exe with standard icons"
if errorlevel 1 goto build_failed

call :build "%GUI%" "7zG.exe"
if errorlevel 1 goto build_failed

call :build "%SFX%" "7z.sfx"
if errorlevel 1 goto build_failed

call :copy_standalone_outputs "%PLATFORM%-bin"
call :copy_docs "%PLATFORM%-bin"

if not "%MODE%"=="" goto restore_darkmode
goto build_fluent_variant

:build_full
echo Compiling full
pushd "CPP\7zip"
nmake
set BUILD_ERROR=%errorlevel%
popd
if not "%BUILD_ERROR%"=="0" (
  echo Build failed with full
  goto build_failed
)
goto restore_darkmode

:build_all
echo Compiling all
pushd "CPP\7zip"
nmake
set BUILD_ERROR=%errorlevel%
popd
if not "%BUILD_ERROR%"=="0" (
  echo Build failed with all
  goto build_failed
)

call :copy_standalone_outputs "%PLATFORM%-bin"
call :copy_docs "%PLATFORM%-bin"
goto build_fluent_variant

:build_fluent_variant
rem Fluent builds are supported only for x64 or arm64.
if "%PLATFORM%"=="x86" goto restore_darkmode

call :del_fm
call :build_fluent "%FM%" "7zFM.exe with fluent icons"
if errorlevel 1 goto build_failed

call :build "%GUI%" "7zG.exe"
if errorlevel 1 goto build_failed

call :build "%SFX%" "7z.sfx"
if errorlevel 1 goto build_failed

call :copy_standalone_outputs "%PLATFORM%-fluent-bin"
call :copy_docs "%PLATFORM%-fluent-bin"

if "%MODE%"=="installer" goto build_installer
if "%MODE%"=="" goto build_installer
goto restore_darkmode

:build_installer
call :build "%FMT%" "7z.dll"
if errorlevel 1 goto build_failed

call :build "%CONS%" "7z.exe"
if errorlevel 1 goto build_failed

call :build "%SFXC%" "7zCon.sfx"
if errorlevel 1 goto build_failed

call :build "%EXP%" "7-zip.dll"
if errorlevel 1 goto build_failed

call :build "C\Util\7zipInstall" "7zipInstall.exe"
if errorlevel 1 goto build_failed

call :build "C\Util\7zipUninstall" "7zipUninstall.exe"
if errorlevel 1 goto build_failed

if not exist "%PLATFORM%-fluent-installer-bin\" mkdir "%PLATFORM%-fluent-installer-bin"

call :copy_standalone_outputs "%PLATFORM%-fluent-installer"
for %%A in (
  "%FMT%\%PLATFORM%\7z.dll"
  "%CONS%\%PLATFORM%\7z.exe"
  "%SFXC%\%PLATFORM%\7zCon.sfx"
  "%EXP%\%PLATFORM%\7-zip.dll"
) do xcopy "%%~A" "%PLATFORM%-fluent-installer" /Y >nul

copy "C\Util\7zipUninstall\%PLATFORM%\7zipUninstall.exe" "%PLATFORM%-fluent-installer\Uninstall.exe" >nul

if not exist "docs-installer\" mkdir "docs-installer"

pushd "docs-installer"
echo Downloading %URL%
curl %URL% -L -o 7-Zip.exe
"..\%PLATFORM%-fluent-installer\7z.exe" x 7-Zip.exe
del /F /Q *.exe *.dll *.sfx
popd

xcopy "docs-installer\" "%PLATFORM%-fluent-installer" /E /I /Y >nul
rmdir /S /Q "docs-installer"

rem vcvarsall will overwite PLATFORM so save it to temp variable and then restore it
set TARGET_PLATFORM=%PLATFORM%

if "%PLATFORM%"=="x64" (
  call "%InstallDir%\VC\Auxiliary\Build\vcvarsall.bat" x64_x86

  call :build "%EXP%" "7-zip32.dll"
  if errorlevel 1 goto build_failed

  copy "%EXP%\x86\7-zip.dll" "%PLATFORM%-fluent-installer\7-zip32.dll" >nul
)

set PLATFORM=%TARGET_PLATFORM%

pushd "%PLATFORM%-fluent-installer"
"7z.exe" a "..\%PLATFORM%-fluent-installer-bin\%PLATFORM%-fluent-installer.7z" -m0=lzma -mx9 -ms=on -mf=bcj2
if errorlevel 1 (
  popd
  goto build_failed
)
popd
rmdir /S /Q "%PLATFORM%-fluent-installer"

rem Make installer
echo Creating installer
copy /b ".\C\Util\7zipInstall\%PLATFORM%\7zipInstall.exe" /b + ".\%PLATFORM%-fluent-installer-bin\%PLATFORM%-fluent-installer.7z" /b ".\%PLATFORM%-fluent-installer-bin\7z%VERSION%-dark-%PLATFORM%.exe" >nul
del /F /Q "%PLATFORM%-fluent-installer-bin\%PLATFORM%-fluent-installer.7z"

call :copy_docs "%PLATFORM%-fluent-installer-bin"
goto restore_darkmode

:build_failed
echo Build failed. Restoring DarkMode\lib\src\StdAfx.h...
goto restore_darkmode_error

:restore_darkmode
rem Restore original non-7z darkmodelib StdAfx.h
xcopy "tmp\StdAfx.h" "DarkMode\lib\src" /Y >nul
rmdir /S /Q "tmp"
endlocal
exit /b 0

:restore_darkmode_error
xcopy "tmp\StdAfx.h" "DarkMode\lib\src" /Y >nul
rmdir /S /Q "tmp"
endlocal
exit /b 1

@rem Helpers
:copy_standalone_outputs
set DEST=%1
if not exist "%DEST%\" mkdir "%DEST%"
for %%A in (
  "%SFX%\%PLATFORM%\7z.sfx"
  "%FM%\%PLATFORM%\7zFM.exe"
  "%GUI%\%PLATFORM%\7zG.exe"
) do xcopy "%%~A" "%DEST%" /Y >nul
exit /b

:copy_docs
set DEST=%1
for %%A in (
  "%DARK%\7zDark.ini"
  "LICENSE.md"
) do xcopy "%%~A" "%DEST%" /Y >nul
exit /b

:del_fm
rem Delete FM output only when building on x64 or arm64, due to fluent icons.
if not "%PLATFORM%"=="x86" (
  if exist "%FM%\%PLATFORM%" rmdir /S /Q "%FM%\%PLATFORM%"
)
exit /b

:build
echo Compiling %2
pushd %1
nmake
set BUILD_ERROR=%errorlevel%
popd
if not "%BUILD_ERROR%"=="0" (
  echo Build failed with %2
  exit /b 1
)
exit /b 0

:build_fluent
echo Compiling %2
pushd %1
nmake DARKMODE_FLUENT_ICON=1
set BUILD_ERROR=%errorlevel%
popd
if not "%BUILD_ERROR%"=="0" (
  echo Build failed with %2
  exit /b 1
)
exit /b 0
