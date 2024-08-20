# Dark7zip

[![7z](https://img.shields.io/badge/7z-DarkMode-black.svg?&logo=7zip)](https://github.com/ozone10/7zip-Dark7zip)
[![Latest release](https://img.shields.io/github/v/release/ozone10/7zip-Dark7zip?include_prereleases)](https://github.com/ozone10/7zip-Dark7zip/releases/latest)
[![Total downloads](https://img.shields.io/github/downloads/ozone10/7zip-Dark7zip/total.svg)](https://github.com/ozone10/7zip-Dark7zip/releases)
[![License](https://img.shields.io/github/license/ozone10/7zip-Dark7zip?color=9cf)](https://www.gnu.org/licenses/gpl-3.0.en.html)
---

Dark7zip is project to experiment with dark mode using win32 API.

* * *

<p align="center">
  <img src="https://i.imgur.com/cnYZPAE.png">
</p>

* * *

## Installation

Replace 7z original files (e.g. `C:\Program Files\7-Zip\`) with files from downloaded zip file (currently only for x64 platform).

- `7zFM.exe` - file manager, "main exe"
- `7zG.exe` - mainly extraction, compression dialogs
- `7z.sfx`- optional, file needed for creating self-extracting archives
- `7zDark.ini` - optional, more information below

## Config

`7zDark.ini` is configuration file to allow mainly to set custom colors.
`7zDark.ini` should be in same folder as `7zFM.exe` and `7zG.exe`.

- [main]
  - mode - determine which sections color "key=value" pairs will be used and theming of title bar, buttons, scrollbars, and tooltips
      - 0 - use light mode 
      - 1 - use dark mode, default value
      - 2 - follow system settings

- [dark]
  - tone - set default colors for [dark.colors] sections, there are no tones for light mode
    - 0 - black, default value
    - 1 - red
    - 2 - green
    - 3 - blue
    - 4 - purple
    - 5 - cyan
    - 6 - olive

Values for custom colors are in RGB hex format - RRGGBB.

- [dark.colors]/[light.colors]
  - background - for some controls (status bar, ...)
  - backgroundInteractive - mainly for controls with some input 
  - backgroundHot - for hot item
  - backgroundDlg - for dialog and some controls
  - text - for most text
  - textItem - for some controls
  - textDisabled - for disabled controls
  - edge - for border
  - edgeHot - for hot border
  - edgeDisabled - for disabled border

- [dark.colors.view]/[light.colors.view] - for listview and treeview
  - backgroundView
  - textView
  - gridlines - color for listview grid lines, when View -> Details is used and Tools -> Options... -> Settings tab -> Show grid lines is checked

All options are optional. You can define only "[section]" and "key=value" you want to be applied.  
Tip: renaming section will disable all "key=value" pairs of that section and default values will be used.

Examples:
- full config
```ini
[main]
mode = 1

[dark]
tone = 0

[dark.colors]
background =            "202020"
backgroundInteractive = "404040"
backgroundHot =         "404040"
backgroundDlg =         "202020"
text =                  "E0E0E0"
textItem =              "C0C0C0"
textDisabled =          "808080"
edge =                  "646464"
edgeHot =               "9B9B9B"
edgeDisabled =          "484848"

[dark.colors.view]
backgroundView =        "293134"
textView =              "E0E2E4"
gridlines =             "646464"

[light.colors]
background =            "F0F0F0"
backgroundInteractive = "FFFFFF"
backgroundHot =         "C0DCF3"
backgroundDlg =         "F0F0F0"
text =                  "000000"
textItem =              "000000"
textDisabled =          "6D6D6D"
edge =                  "8D8D8D"
edgeHot =               "0078D4"
edgeDisabled =          "6D6D6D"

[light.colors.view]
backgroundView =        "FFFFFF"
textView =              "000000"
gridlines =             "F0F0F0"
```

- only views, with defaults: `[main]` `mode=1`, and `[dark]` `tone=0`
```ini
[dark.colors.view]
backgroundView =        "112435"
textView =              "C3BE98"
gridlines =             "4F5F5F"
```


## License

Most code related to dark mode is under license MIT, or GPLv3 or later version.
  
For 7-Zip below is information for used license.
<details>
  <summary>license.txt</summary>

  ```txt
  7-Zip
  ~~~~~
  License for use and distribution
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  7-Zip Copyright (C) 1999-2023 Igor Pavlov.

  The licenses for files are:

    1) 7z.dll:
         - The "GNU LGPL" as main license for most of the code
         - The "GNU LGPL" with "unRAR license restriction" for some code
         - The "BSD 3-clause License" for some code
    2) All other files: the "GNU LGPL".

  Redistributions in binary form must reproduce related license information from this file.

  Note:
    You can use 7-Zip on any computer, including a computer in a commercial
    organization. You don't need to register or pay for 7-Zip.


  GNU LGPL information
  --------------------

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You can receive a copy of the GNU Lesser General Public License from
    http://www.gnu.org/




  BSD 3-clause License
  --------------------

    The "BSD 3-clause License" is used for the code in 7z.dll that implements LZFSE data decompression.
    That code was derived from the code in the "LZFSE compression library" developed by Apple Inc,
    that also uses the "BSD 3-clause License":

    ----
    Copyright (c) 2015-2016, Apple Inc. All rights reserved.

    Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    1.  Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

    2.  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
        in the documentation and/or other materials provided with the distribution.

    3.  Neither the name of the copyright holder(s) nor the names of any contributors may be used to endorse or promote products derived
        from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
    COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
    ----




  unRAR license restriction
  -------------------------

    The decompression engine for RAR archives was developed using source
    code of unRAR program.
    All copyrights to original unRAR code are owned by Alexander Roshal.

    The license for original unRAR code has the following restriction:

      The unRAR sources cannot be used to re-create the RAR compression algorithm,
      which is proprietary. Distribution of modified unRAR sources in separate form
      or as a part of other software is permitted, provided that it is clearly
      stated in the documentation and source comments that the code may
      not be used to develop a RAR (WinRAR) compatible archiver.


  --
  Igor Pavlov

  ```

  </details>

<details>
  <summary>readme.txt</summary>

  ```txt
  7-Zip 24.07 Sources
-------------------

7-Zip is a file archiver for Windows. 

7-Zip Copyright (C) 1999-2024 Igor Pavlov.


License Info
------------

7-Zip is free software distributed under the GNU LGPL 
(except for unRar code). Also some  code
is licensed under the "BSD 3-clause License".
Read "License.txt" for more infomation about license.

Notes about unRAR license:

Please check main restriction from unRar license:

   2. The unRAR sources may be used in any software to handle RAR
      archives without limitations free of charge, but cannot be used
      to re-create the RAR compression algorithm, which is proprietary.
      Distribution of modified unRAR sources in separate form or as a
      part of other software is permitted, provided that it is clearly
      stated in the documentation and source comments that the code may
      not be used to develop a RAR (WinRAR) compatible archiver.

In brief it means:
1) You can compile and use compiled files under GNU LGPL rules, since 
   unRAR license almost has no restrictions for compiled files.
   You can link these compiled files to LGPL programs.
2) You can fix bugs in source code and use compiled fixed version.
3) You can not use unRAR sources to re-create the RAR compression algorithm.


LZMA SDK
--------

This package also contains some files from LZMA SDK
You can download LZMA SDK from:
  http://www.7-zip.org/sdk.html
LZMA SDK is written and placed in the public domain by Igor Pavlov.


How to compile in Windows
-------------------------

To compile the sources to Windows binaries you need Visual Studio compiler and/or Windows SDK.
You can use latest Windows Studio 2017/2019/2022 to compile binaries for x86, x64, arm64 and arm platforms.
Also you can use old compilers for some platforms:
  x86   : Visual C++ 6.0 with Platform SDK
  x64   : Windows Server 2003 R2 Platform SDK
  ia64 (itanium)  : Windows Server 2003 R2 Platform SDK
  arm for Windows CE : Standard SDK for Windows CE 5.0

If you use MSVC6, specify also Platform SDK directories at top of directories lists:
Tools / Options / Directories
  - Include files
  - Library files

Also you need Microsoft Macro Assembler:
  - ml.exe for x86 
  - ml64.exe for x64
You can use ml.exe from Windows SDK for Windows Vista or some later versions.

There are two ways to compile 7-Zip binaries:
1) via makefile in command line.
2) via dsp file in Visual Studio.

The dsp file compiling can be used for development and debug purposes.
All final 7-Zip binaries are compiled via makefiles, that provide best
optimization options.


How to compile with makefile
----------------------------

Some macronames can be defined for compiling with makefile:

PLATFORM
  with possible values: x64, x86, arm64, arm, ia64

OLD_COMPILER
  for old VC compiler, like MSCV 6.0.

MY_DYNAMIC_LINK
  for dynamic linking to the run-time library (msvcrt.dll). 
  The default makefile option is static linking to the run-time library.



Compiling 7-Zip for Unix/Linux
------------------------------

There are several options to compile 7-Zip with different compilers: gcc and clang.
Also 7-Zip code contains two versions for some parts of code: in C and in Assembeler.
So if you compile the version with Assembeler code, you will get faster 7-Zip binary.

7-Zip's assembler code uses the following syntax for different platforms:

1) x86 and x86-64 (AMD64): MASM syntax. 
   There are 2 programs that supports MASM syntax in Linux.
'    'Asmc Macro Assembler and JWasm. But JWasm now doesn't support some 
      cpu instructions used in 7-Zip.
   So you must install Asmc Macro Assembler in Linux, if you want to compile fastest version
   of 7-Zip  x86 and x86-64:
     https://github.com/nidud/asmc

2) arm64: GNU assembler for ARM64 with preprocessor. 
   That systax is supported by GCC and CLANG for ARM64.

There are different binaries that can be compiled from 7-Zip source.
There are 2 main files in folder for compiling:
  makefile        - that can be used for compiling Windows version of 7-Zip with nmake command
  makefile.gcc    - that can be used for compiling Linux/macOS versions of 7-Zip or Windows version 
                    with MINGW (GCC) with make command.
                   
At first you must change the current folder to folder that contains `makefile.gcc`:

  cd CPP/7zip/Bundles/Alone2

Then you can compile `makefile.gcc` with the command:

  make -j -f makefile.gcc

Also there are additional "*.mak" files in folder "CPP/7zip/" that can be used to compile 
7-Zip binaries with optimized code and optimzing options.

To compile with GCC without assembler:
  cd CPP/7zip/Bundles/Alone2
  make -j -f ../../cmpl_gcc.mak

To compile with CLANG without assembler:
  make -j -f ../../cmpl_clang.mak

To compile 7-Zip for x86-64 with asmc assembler:
  make -j -f ../../cmpl_gcc_x64.mak

To compile 7-Zip for arm64 with assembler:
  make -j -f ../../cmpl_gcc_arm64.mak

To compile 7-Zip for arm64 for macOS:
  make -j -f ../../cmpl_mac_arm64.mak

Also you can change some compiler options in the "mak" files:
  cmpl_gcc.mak
  var_gcc.mak
  warn_gcc.mak

makefile.gcc supports some variables that can change compile options

USE_JWASM=1
  use JWasm assembler instead of Asmc.
  Note that JWasm doesn't support AES instructions. So AES code from C version AesOpt.c 
  will be used instead of assembler code from AesOpt.asm.

DISABLE_RAR=1
  removes whole RAR related code from compilation.

DISABLE_RAR_COMPRESS=1
  removes "not fully free" code of RAR decompression codecs from compilation.

RAR decompression codecs in 7-Zip code has some additional license restrictions, 
that can be treated as not fully compatible with free-software licenses.
DISABLE_RAR_COMPRESS=1 allows to exclude such "not-fully-free" RAR code from compilation.
if DISABLE_RAR_COMPRESS=1 is specified, 7-zip will not be able to decompress files 
from rar archives, but 7-zip still will be able to open rar archives to get list of 
files or to extract files that are stored without compression.
if DISABLE_RAR=1 is specified, 7-zip will not be able to work with RAR archives.



7-Zip and p7zip
===============
Now there are two different ports of 7-Zip for Linux/macOS:

1) p7zip - another port of 7-Zip for Linux, made by an independent developer.
   The latest version of p7zip now is 16.02, and that p7zip 16.02 is outdated now.
   http://sourceforge.net/projects/p7zip/ 

2) 7-Zip for Linux/macOS - this package - it's new code with all changes from latest 7-Zip for Windows.

These two ports are not identical. 
Note also that some Linux specific things can be implemented better in p7zip than in new 7-Zip for Linux.




Notes:
------
7-Zip consists of COM modules (DLL files).
But 7-Zip doesn't use standard COM interfaces for creating objects.
Look at
7zip\UI\Client7z folder for example of using DLL files of 7-Zip. 
Some DLL files can use other DLL files from 7-Zip.
If you don't like it, you must use standalone version of DLL.
To compile standalone version of DLL you must include all used parts
to project and define some defs. 
For example, 7zip\Bundles\Format7z is a standalone version  of 7z.dll 
that works with 7z format. So you can use such DLL in your project 
without additional DLL files.


Description of 7-Zip sources package
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

DOC                Documentation
---
  readme.txt     - Readme file
  src-history.txt  - Sources history
  7zC.txt        - 7z ANSI-C Decoder description
  7zFormat.txt   - 7z format description
  Methods.txt    - Compression method IDs
  lzma.txt       - LZMA compression description
  License.txt    - license information
  copying.txt    - GNU LGPL license
  unRarLicense.txt - License for unRAR part of source code
  7zip.wxs       - installer script for WIX
  7zip.hhp       - html help project file

Asm - Source code in Assembler : optimized code for CRC, SHA, AES, LZMA decoding.

C   - Source code in C

CPP - Source code in C++

Common            common files for C++ projects

Windows           common files for Windows related code

7zip

  Common          Common modules for 7-zip

  Archive         files related to archiving

  Bundle          Modules that are bundles of other modules (files)

    Alone         7za.exe: Standalone version of 7-Zip console that supports only 7z/xz/cab/zip/gzip/bzip2/tar.
    Alone2        7zz.exe: Standalone version of 7-Zip console that supports all formats.
    Alone7z       7zr.exe: Standalone version of 7-Zip console that supports only 7z (reduced version)
    Fm            Standalone version of 7-Zip File Manager
    Format7z            7za.dll:  .7z support
    Format7zExtract     7zxa.dll: .7z support, extracting only
    Format7zR           7zr.dll:  .7z support, reduced version
    Format7zExtractR    7zxr.dll: .7z support, reduced version, extracting only
    Format7zF           7z.dll:   all formats
    LzmaCon       lzma.exe: LZMA compression/decompression
    SFXCon        7zCon.sfx: Console 7z SFX module
    SFXWin        7z.sfx: Windows 7z SFX module
    SFXSetup      7zS.sfx: Windows 7z SFX module for Installers

  Compress        files for compression / decompression

  Crypto          files for encryption / decryption

  UI

    Agent         Intermediary modules for FAR plugin and Explorer plugin
    Client7z      Test application for 7za.dll 
    Common        Common UI files
    Console       7z.exe : Console version
    Explorer      7-zip.dll: 7-Zip Shell extension
    Far           plugin for Far Manager
    FileManager   7zFM.exe: 7-Zip File Manager
    GUI           7zG.exe: 7-Zip GUI version



---
Igor Pavlov
http://www.7-zip.org

  ```

</details>


## 7-Zip on GitHub
7-Zip website: [7-zip.org](https://7-zip.org)
