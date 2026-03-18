@echo off

cd /d "%~dp0"

setlocal EnableDelayedExpansion

set VERSION=2600

rem Platform
set PLATFORM=%1
set URL=https://www.7-zip.org/a/7z%VERSION%-%PLATFORM%.exe

if "%PLATFORM%" == "x64" (
  set ARCH=x64
) else if "%PLATFORM%" == "x86" (
  set ARCH=x64_x86
  set URL=https://www.7-zip.org/a/7z%VERSION%.exe
) else if "%PLATFORM%" == "arm64" (
  set ARCH=x64_arm64
) else (
  set ARCH=x64
  set PLATFORM=x64
  set URL=https://www.7-zip.org/a/7z%VERSION%-%PLATFORM%.exe
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

rem  Clean
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

  if "%PLATFORM%" == "x64" (
    if exist "%EXP%\x86" rmdir /S /Q "%EXP%\x86"
  )

  echo Clean complete.
  exit /b
)

if not exist "DarkMode\lib\src\StdAfx.h" (
  echo DarkMode\lib\src\StdAfx.h not found. Did you get darkmodelib submodule?
  exit /b 1
)

call :del_res

if not exist "tmp\" mkdir "tmp"
move "DarkMode\lib\src\StdAfx.h" "tmp" >nul
xcopy "%DARK%\StdAfx.h" "DarkMode\lib\src" /y >nul

if "%2" == "all" (
  echo Compiling all
  pushd "CPP\7zip"
  nmake
  popd
) else (
  rem Build only relevant binaries
  if "%2" == "" (
    call :build "%FM%" "7zFM.exe with standard icons"
  )

  call :build "%GUI%" "7zG.exe"
  call :build "%SFX%" "7z.sfx"
)

if not "%2" == "fluent" (
  call :copy_standalone_outputs "%PLATFORM%-bin"
  call :copy_docs "%PLATFORM%-bin"
)

rem Build the fluent version only for x64 or arm64
if not "%PLATFORM%" == "x86" (
  if not exist "tmp\" mkdir "tmp"
  move "%FM%\*.bmp" "tmp" >nul
  xcopy "%DARK%\icons\*.bmp" "%FM%" /y >nul

  call :del_res

  call :build "%FM%" "7zFM.exe with fluent icons"

  call :del_res

  call :copy_standalone_outputs "%PLATFORM%-fluent-bin"
  call :copy_docs "%PLATFORM%-fluent-bin"

  rem Installer part
  if not "%2" == "all" (
    call :build "%FMT%" "7z.dll"
    call :build "%CONS%" "7z.exe"
    call :build "%SFXC%" "7zCon.sfx"
    call :build "%EXP%" "7-zip.dll"
    call :build "C\Util\7zipInstall" "7zipInstall.exe"
    call :build "C\Util\7zipUninstall" "7zipUninstall.exe"
  )

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

  if "%PLATFORM%" == "x64" (
    call "%InstallDir%\VC\Auxiliary\Build\vcvarsall.bat" x64_x86

    call :build "%EXP%" "7-zip32.dll"

    copy "%EXP%\x86\7-zip.dll" "%PLATFORM%-fluent-installer\7-zip32.dll" >nul
  )

  pushd "%PLATFORM%-fluent-installer"
  "7z.exe" a "..\%PLATFORM%-fluent-installer-bin\%PLATFORM%-fluent-installer.7z" -m0=lzma -mx9 -ms=on -mf=bcj2
  popd
  rmdir /S /Q "%PLATFORM%-fluent-installer"

  rem Make installer
  echo Creating installer
  copy /b ".\C\Util\7zipInstall\%PLATFORM%\7zipInstall.exe" /b + ".\%PLATFORM%-fluent-installer-bin\%PLATFORM%-fluent-installer.7z" /b ".\%PLATFORM%-fluent-installer-bin\7z%VERSION%-dark-%PLATFORM%.exe" >nul
  del  /F /Q "%PLATFORM%-fluent-installer-bin\%PLATFORM%-fluent-installer.7z"

  call :copy_docs "%PLATFORM%-fluent-installer-bin"

  rem Restore icons
  xcopy "tmp\*.bmp" "%FM%" /Y >nul
)

rem Restore original non-7z darkmodelib StdAfx.h
xcopy "tmp\StdAfx.h" "DarkMode\lib\src" /Y >nul
rmdir /S /Q "tmp"

rem pause

endlocal
exit /b

@rem Helper
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

:del_res
if not "%2" == "fluent" (
  rem Delete resource.res only if x64 or arm64 is used, due to fluent version
  if not "%PLATFORM%"=="x86" (
    if exist "%FM%\%PLATFORM%\resource.res" (
      del /F /Q "%FM%\%PLATFORM%\resource.res"
    )
  )
)
exit /b

:build
echo Compiling %2
pushd %1
nmake
popd
if errorlevel 1 (
  echo Build failed with %2
  exit /b 1
)
exit /b
