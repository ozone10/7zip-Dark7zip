# Dark7zip

[![7z](https://img.shields.io/badge/7z-DarkMode-black.svg?&logo=7zip)](https://github.com/ozone10/7zip-Dark7zip)
[![Build status](https://img.shields.io/github/actions/workflow/status/ozone10/7zip-Dark7zip/build_win.yml?logo=Github)](https://github.com/ozone10/7zip-Dark7zip/actions)
[![Latest release](https://img.shields.io/github/v/release/ozone10/7zip-Dark7zip?include_prereleases)](https://github.com/ozone10/7zip-Dark7zip/releases/latest)
[![Total downloads](https://img.shields.io/github/downloads/ozone10/7zip-Dark7zip/total.svg)](https://github.com/ozone10/7zip-Dark7zip/releases)
[![License](https://img.shields.io/github/license/ozone10/7zip-Dark7zip?color=green)](https://www.gnu.org/licenses/gpl-3.0.en.html)
[![License-MIT](https://img.shields.io/badge/license-MIT-green)](./LICENSE-MIT.md)
[![PayPal.me](https://img.shields.io/badge/PayPal-me-blue.svg?maxAge=2592000)](https://paypal.me/ozone10/)
---

Dark7zip is project to experiment with dark mode for [7-Zip](https://github.com/ip7z/7zip) and its fork [7-Zip-zstd](#7-zip-zstd).  
It is mainly for Windows 10 and Windows 11. Some controls might, might not use dark/custom colors on older OS.

* * *

<p align="center">
  <img src="https://i.imgur.com/pghAZEA.png">
</p>

<details>
  <summary>Gallery</summary>

  <p align="center">
    <img src="https://i.imgur.com/cnYZPAE.png">
  </p>

  <p align="center">
    <img src="https://i.imgur.com/6tNDlFM.png">
  </p>

  <p align="center">
    <img src="https://i.imgur.com/Ddc4z7O.png">
  </p>

  <p align="center">
    <img src="https://i.imgur.com/6a4lu9U.png">
  </p>
</details>

* * *

## Installation

Replace 7z original files (e.g. `C:\Program Files\7-Zip\`) with files from downloaded zip file.  

- `7zFM.exe` - file manager, "main exe"
- `7zG.exe` - mainly extraction, compression dialogs
- `7z.sfx`- optional, file needed for creating self-extracting archives
- `7zDark.ini` - optional, more information below

> [!IMPORTANT]  
> Ensure that you have downloaded the correct version. If the zip file name contains "zstd" (e.g. `7z-zstd-v24.09-v1.5.7-R1-dark-x64.zip`), it is intended for the zstd fork. If you are using the wrong version, a message box titled "7‑Zip ZS" will appear when attempting to extract the archive.

> [!NOTE]  
> Currently files are released only for x64 platform. You can get files for other platform from Github Actions for original 7-Zip, 7-Zip-zstd has only x64 platform. Ensure that you get correct version - [main](https://github.com/ozone10/7zip-Dark7zip/actions?query=branch%3Amain) for original 7-Zip, [7z-zstd](https://github.com/ozone10/7zip-Dark7zip/actions?query=branch%3A7z-zstd) for 7-Zip-zstd.

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
  - micaExtend - apply Mica material on all window/dialog, experimental, only for "dark" mode
    - 0 - apply Mica material only on title bar, default value
    - 1 - extend Mica material on all window/dialog

> [!IMPORTANT]  
> `micaExtend=1` is only used with `[main]` `mode=1` and with `mica` with other valid values than `0`.
> `micaExtend=1` should also be used with HDR and ACM (Auto Color Management) off.
> Due to Windows bug using `micaExtend=1` can cause visual glitches, with HDR/ACM visual glitches are more severe (e.g. invisible controls).  
> It is also recommended when using with `mica=1` to turn off Settings -> Personalization > Colors -> "Show accent color on title bars and window borders" setting.

- [dark]/[light]
  - roundCorner - on Windows 11 specifies rounded corners preference
    - 0 - let the system decide when to round window corners, default value
    - 1 - never round window corners
    - 2 - round the corners, if appropriate
    - 3 - round the corners if appropriate, with a small radius
  - borderColor - on Windows 11 specifies the color of the window border, color is in RGB hex format - RRGGBB
    - FFFFFF - use default system color, special value, default value
  - mica - apply Mica material on title bar, requires Windows 11 22H2 build 22621
    - 0 - let OS automatically decide the use of Mica material, default value
    - 1 - do not use Mica material
    - 2 - apply Mica material
    - 3 - apply "acrylic" effect
    - 4 - apply Mica Alt material

Values for custom colors are in RGB hex format - RRGGBB.

- [dark.colors]/[light.colors]
  - background - for some controls (status bar, ...)
  - backgroundCtrl - mainly for controls with some input 
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
  - backgroundHeader
  - backgroundHotHeader
  - textHeader
  - edgeHeader

All options are optional. You can define only "[section]" and "key=value" you want to be applied.  

> [!TIP]  
> Renaming section will disable all "key=value" pairs of that section and default values will be used.

Examples:
- full config
```ini
[main]
mode = 1

[dark]
tone = 0
roundCorner = 0
borderColor = "FFFFFF"
mica = 0
micaExtend = 0

[dark.colors]
background =            "202020"
backgroundCtrl =        "404040"
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
backgroundHeader =      "202020"
backgroundHotHeader =   "404040"
textHeader =            "C0C0C0"
edgeHeader =            "646464"

[light]
roundCorner = 0
borderColor = "FFFFFF"
mica = 0

[light.colors]
background =            "F0F0F0"
backgroundCtrl =        "FFFFFF"
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
backgroundHeader =      "FFFFFF"
backgroundHotHeader =   "D9EBF9"
textHeader =            "000000"
edgeHeader =            "E5E5E5"
```

- only views, with defaults: `[main]` `mode=1`, and `[dark]` `tone=0`
```ini
[dark.colors.view]
backgroundView =        "112435"
textView =              "C3BE98"
gridlines =             "4F5F5F"
```

## 7-Zip-zstd

7-Zip-zstd is popular 7-Zip fork which contains additional compression algorithms such as Zstandard and Brotli.
Source code for dark mode for it is in [7z-zstd](https://github.com/ozone10/7zip-Dark7zip/tree/7z-zstd) branch.

For more information about 7-Zip-zstd fork check [7-Zip-zstd](https://github.com/mcmilk/7-Zip-zstd).

## License

Most code related to dark mode is under license MIT, or GPLv3 or later version.

For 7-Zip check DOC folder for information on used license.  
[License.txt](DOC/License.txt)  
[readme.txt](DOC/readme.txt)

## 7-Zip on GitHub
7-Zip website: [7-zip.org](https://7-zip.org)
