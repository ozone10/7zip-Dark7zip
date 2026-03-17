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

set VERSION=2600
SET URL=https://www.7-zip.org/a/7z2600.exe

set PLATFORM=%1

if "%PLATFORM%" == "x64" (
  set ARCH=x64
) else if "%PLATFORM%" == "x86" (
  set ARCH=x64_x86
) else if "%PLATFORM%" == "arm64" (
  set ARCH=x64_arm64
) else (
  set ARCH=x64
  set PLATFORM=x64
)

call "%InstallDir%\VC\Auxiliary\Build\vcvarsall.bat" %ARCH%

if not "%2" == "fluent" (
  rem Delete resource.res only if x64 or arm64 is used, due to fluent version
  if not "%PLATFORM%"=="x86" (
    if exist "CPP\7zip\UI\FileManager\%PLATFORM%\resource.res" (
      del /F /Q "CPP\7zip\UI\FileManager\%PLATFORM%\resource.res"
    )
  )
)

if not exist "tmp\" mkdir "tmp"
move "DarkMode\lib\src\StdAfx.h" "tmp" >nul
xcopy "DarkMode\7zRes\StdAfx.h" "DarkMode\lib\src" /y >nul

if "%2" == "all" (
  echo Compiling all
  pushd CPP\7zip
  nmake
  popd
) else (
  rem Build only relevant binaries
  if "%2" == "" (
    echo Compiling 7zFM.exe with standard icons
    pushd CPP\7zip\UI\FileManager
    nmake
    popd
  )

  echo Compiling 7zG.exe
  pushd CPP\7zip\UI\GUI
  nmake
  popd

  echo Compiling 7z.sfx
  pushd CPP\7zip\Bundles\SFXWin
  nmake
  popd
)

if not "%2" == "fluent" (
  if not exist "%PLATFORM%-bin\" mkdir "%PLATFORM%-bin"
  for %%A in (
    "CPP\7zip\Bundles\SFXWin\%PLATFORM%\7z.sfx"
    "CPP\7zip\UI\FileManager\%PLATFORM%\7zFM.exe"
    "CPP\7zip\UI\GUI\%PLATFORM%\7zG.exe"
    "DarkMode\7zRes\7zDark.ini"
    "LICENSE.md"
  ) do copy "%%~A" "%PLATFORM%-bin" >nul
)

rem Build the fluent version only for x64 or arm64
if not "%PLATFORM%" == "x86" (
  if not exist "tmp\" mkdir "tmp"
  move "CPP\7zip\UI\FileManager\*.bmp" "tmp" >nul
  xcopy "DarkMode\7zRes\icons\*.bmp" "CPP\7zip\UI\FileManager" /y >nul

  if not "%2" == "fluent" (
    if exist "CPP\7zip\UI\FileManager\%PLATFORM%\resource.res" (
      del /F /Q "CPP\7zip\UI\FileManager\%PLATFORM%\resource.res"
    )
  )

  echo Compiling 7zFM.exe with fluent icons
  pushd CPP\7zip\UI\FileManager
  nmake
  popd

  if not exist "%PLATFORM%-fluent-bin\" mkdir "%PLATFORM%-fluent-bin"
  for %%A in (
    "CPP\7zip\Bundles\SFXWin\%PLATFORM%\7z.sfx"
    "CPP\7zip\UI\FileManager\%PLATFORM%\7zFM.exe"
    "CPP\7zip\UI\GUI\%PLATFORM%\7zG.exe"
    "DarkMode\7zRes\7zDark.ini"
    "LICENSE.md"
  ) do copy "%%~A" "%PLATFORM%-fluent-bin" >nul

  rem Installer part
  if not "%2" == "all" (
    echo Compiling 7z.dll
    pushd CPP\7zip\Bundles\Format7zF
    nmake
    popd

    echo Compiling 7z.exe
    pushd CPP\7zip\UI\Console
    nmake
    popd

    echo Compiling 7zCon.sfx
    pushd CPP\7zip\Bundles\SFXCon
    nmake
    popd

    echo Compiling 7-zip.dll
    pushd CPP\7zip\UI\Explorer
    nmake
    popd

    echo Compiling 7zipInstall.exe
    pushd C\Util\7zipInstall
    nmake
    popd

    echo Compiling 7zipUninstall.exe
    pushd C\Util\7zipUninstall
    nmake
    popd
  )

  if not exist "%PLATFORM%-fluent-installer\" mkdir "%PLATFORM%-fluent-installer"
  if not exist "%PLATFORM%-fluent-installer-bin\" mkdir "%PLATFORM%-fluent-installer-bin"

  for %%A in (
    "CPP\7zip\Bundles\SFXWin\%PLATFORM%\7z.sfx"
    "CPP\7zip\UI\FileManager\%PLATFORM%\7zFM.exe"
    "CPP\7zip\UI\GUI\%PLATFORM%\7zG.exe"
    "CPP\7zip\Bundles\Format7zF\%PLATFORM%\7z.dll"
    "CPP\7zip\UI\Console\%PLATFORM%\7z.exe"
    "CPP\7zip\Bundles\SFXCon\%PLATFORM%\7zCon.sfx"
    "CPP\7zip\UI\Explorer\%PLATFORM%\7-zip.dll"
  ) do copy "%%~A" "%PLATFORM%-fluent-installer" >nul

  copy "C\Util\7zipUninstall\%PLATFORM%\7zipUninstall.exe" "%PLATFORM%-fluent-installer\Uninstall.exe"

  if not exist "docs-installer\" mkdir "docs-installer"

  pushd "docs-installer"
  curl %URL% -L -o 7-Zip.exe
  "..\%PLATFORM%-fluent-installer\7z.exe" x 7-Zip.exe
  del /F /Q *.exe *.dll *.sfx
  popd

  robocopy "docs-installer" "%PLATFORM%-fluent-installer" /E >nul
  rmdir /S /Q "docs-installer"

  if "%PLATFORM%" == "x64" (
    call "%InstallDir%\VC\Auxiliary\Build\vcvarsall.bat" x64_x86

    echo Compiling 7-zip.dll
    pushd CPP\7zip\UI\Explorer
    nmake
    popd

    copy "CPP\7zip\UI\Explorer\x86\7-zip.dll" "%PLATFORM%-fluent-installer\7-zip32.dll"
  )

  pushd "%PLATFORM%-fluent-installer"
  "7z.exe" a "..\%PLATFORM%-fluent-installer-bin\%PLATFORM%-fluent-installer.7z" -m0=lzma -mx9 -ms=on -mf=bcj2
  popd
  rmdir /S /Q "%PLATFORM%-fluent-installer"

  rem Make installer
  copy /b ".\C\Util\7zipInstall\%PLATFORM%\7zipInstall.exe" /b + ".\%PLATFORM%-fluent-installer-bin\%PLATFORM%-fluent-installer.7z" /b ".\%PLATFORM%-fluent-installer-bin\7z%VERSION%-dark-%PLATFORM%.exe"
  del  /F /Q "%PLATFORM%-fluent-installer-bin\%PLATFORM%-fluent-installer.7z"

  for %%A in (
    "DarkMode\7zRes\7zDark.ini"
    "LICENSE.md"
  ) do copy "%%~A" "%PLATFORM%-fluent-installer-bin" >nul

  rem Restore icons
  xcopy "tmp\*.bmp" "CPP\7zip\UI\FileManager" /y >nul
)

rem Restore original non-7z darkmodelib StdAfx.h
xcopy "tmp\StdAfx.h" "DarkMode\lib\src" /y >nul
rmdir /S /Q "tmp"

rem pause

endlocal
