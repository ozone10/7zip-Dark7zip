// SPDX-License-Identifier: MPL-2.0

/*
 * Copyright (c) 2025 ozone10
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

// Based on the Notepad++ dark mode code licensed under GPLv3.
// Originally by adzm / Adam D. Walling, with modifications by the Notepad++ team.
// Heavily modified by ozone10 (Notepad++ contributor).
// Used with permission to relicense under the Mozilla Public License, v. 2.0.


#include "StdAfx.h"

#include "DarkModeSubclass.h"

#if !defined(_DARKMODELIB_NOT_USED)

#include <windows.h>

#include <dwmapi.h>
#include <richedit.h>
#include <uxtheme.h>
#include <vsstyle.h>

#if !defined(_DARKMODELIB_NO_INI_CONFIG)
#include <array>
#endif
#include <cmath>
#include <string>

#include "DmlibColor.h"
#include "DmlibDpi.h"
#include "DmlibHook.h"
#if !defined(_DARKMODELIB_NO_INI_CONFIG)
#include "DmlibIni.h"
#endif
#include "DmlibSubclass.h"
#include "DmlibSubclassControl.h"
#include "DmlibSubclassWindow.h"
#include "DmlibWinApi.h"

#include "Version.h"

/**
 * @namespace DarkMode
 * @brief Provides dark mode theming, subclassing, and rendering utilities for most Win32 controls.
 */
namespace DarkMode
{
	/**
	 * @brief Returns library version information or compile-time feature flags.
	 *
	 * Responds to the specified query by returning either:
	 * - Version numbers (`verMajor`, `verMinor`, `verRevision`)
	 * - Build configuration flags (returns `TRUE` or `FALSE`)
	 * - A constant value (`featureCheck`, `maxValue`) used for validation
	 *
	 * @param libInfoType Integer with `LibInfo` enum value specifying which piece of information to retrieve.
	 * @return Integer value:
	 * - Version: as defined by `DM_VERSION_MAJOR`, etc.
	 * - Boolean flags: `TRUE` (1) if the feature is enabled, `FALSE` (0) otherwise.
	 * - `featureCheck`, `maxValue`: returns the numeric max enum value.
	 * - `-1`: for invalid or unhandled enum cases (should not occur in correct usage).
	 *
	 * @see LibInfo
	 */
	int getLibInfo(int libInfoType)
	{
		const auto infoType = static_cast<LibInfo>(libInfoType);
		switch (infoType)
		{
			case LibInfo::maxValue:
			case LibInfo::featureCheck:
			{
				return static_cast<int>(LibInfo::maxValue);
			}

			case LibInfo::verMajor:
			{
				return DM_VERSION_MAJOR;
			}

			case LibInfo::verMinor:
			{
				return DM_VERSION_MINOR;
			}

			case LibInfo::verRevision:
			{
				return DM_VERSION_REVISION;
			}

			case LibInfo::iniConfigUsed:
			{
#if !defined(_DARKMODELIB_NO_INI_CONFIG)
				return TRUE;
#else
				return FALSE;
#endif
			}

			case LibInfo::allowOldOS:
			{
#if defined(_DARKMODELIB_ALLOW_OLD_OS)
				return _DARKMODELIB_ALLOW_OLD_OS;
#else
				return FALSE;
#endif
			}

			case LibInfo::useDlgProcCtl:
			{
#if defined(_DARKMODELIB_DLG_PROC_CTLCOLOR_RETURNS)
				return _DARKMODELIB_DLG_PROC_CTLCOLOR_RETURNS;
#else
				return FALSE;
#endif
			}

			case LibInfo::preferTheme:
			{
#if defined(_DARKMODELIB_PREFER_THEME)
				return _DARKMODELIB_PREFER_THEME;
#else
				return FALSE;
#endif
			}

			case LibInfo::useSBFix:
			{
#if defined(_DARKMODELIB_USE_SCROLLBAR_FIX)
				return _DARKMODELIB_USE_SCROLLBAR_FIX;
#else
				return FALSE;
#endif
			}
		}
		return -1; // should never happen
	}

	/**
	 * @brief Describes how the application responds to the system theme.
	 *
	 * Used to determine behavior when following the system's light/dark mode setting.
	 * - `disabled`: Do not follow system; use manually selected appearance.
	 * - `light`: Follow system mode; apply light theme when system is in light mode.
	 * - `classic`: Follow system mode; apply classic style when system is in light mode.
	 */
	enum class WinMode : unsigned char
	{
		disabled,  ///< Manual - system mode is ignored.
		light,     ///< Use light theme if system is in light mode.
		classic    ///< Use classic style if system is in light mode.
	};

	/**
	 * @struct DarkModeParams
	 * @brief Defines theming and subclassing parameters for child controls.
	 *
	 * Members:
	 * - `m_themeClassName`: Optional theme class name (e.g. `"DarkMode_Explorer"`), or `nullptr` to skip theming.
	 * - `m_subclass`: Whether to apply custom subclassing for dark-mode painting and behavior.
	 * - `m_theme`: Whether to apply a themed visual style to applicable controls.
	 *
	 * Used during enumeration to configure dark mode application on a per-control basis.
	 */
	struct DarkModeParams
	{
		const wchar_t* m_themeClassName = nullptr;
		bool m_subclass = false;
		bool m_theme = false;
	};

	/// Threshold range around 50.0 where TreeView uses classic style instead of light/dark.
	static constexpr double kMiddleGrayRange = 2.0;

	namespace // anonymous
	{
		/// Global struct
		struct
		{
			DWM_WINDOW_CORNER_PREFERENCE m_roundCorner = DWMWCP_DEFAULT;
			COLORREF m_borderColor = DWMWA_COLOR_DEFAULT;
			DWM_SYSTEMBACKDROP_TYPE m_mica = DWMSBT_AUTO;
			COLORREF m_tvBackground = RGB(41, 49, 52);
			double m_lightness = 50.0;
			TreeViewStyle m_tvStylePrev = TreeViewStyle::classic;
			TreeViewStyle m_tvStyle = TreeViewStyle::classic;
			bool m_micaExtend = false;
			bool m_colorizeTitleBar = false;
			DarkModeType m_dmType = DarkModeType::dark;
			WinMode m_windowsMode = WinMode::disabled;
			bool m_isInit = false;
			bool m_isInitExperimental = false;

#if !defined(_DARKMODELIB_NO_INI_CONFIG)
			std::wstring m_iniName;
			bool m_isIniNameSet = false;
			bool m_iniExist = false;
#endif
		} g_dmCfg;
	} // anonymous namespace

	static dmlib_color::Theme& getTheme()
	{
		static dmlib_color::Theme tMain{};
		return tMain;
	}

	/**
	 * @brief Sets the color tone and its color set for the active theme.
	 *
	 * Applies a color tone (e.g. red, blue, olive) its color set.
	 *
	 * @param colorTone The tone to apply (see @ref ColorTone enum).
	 *
	 * @see DarkMode::getColorTone()
	 * @see DarkMode::Theme
	 */
	void setColorTone(int colorTone)
	{
		DarkMode::getTheme().setToneColors(static_cast<ColorTone>(colorTone));
	}

	/**
	 * @brief Retrieves the currently active color tone for the theme.
	 *
	 * @return The currently selected @ref ColorTone value.
	 *
	 * @see DarkMode::setColorTone()
	 */
	int getColorTone()
	{
		return static_cast<int>(DarkMode::getTheme().getColorTone());
	}

	static dmlib_color::ThemeView& getThemeView()
	{
		static dmlib_color::ThemeView tView{};
		return tView;
	}

	COLORREF setBackgroundColor(COLORREF clrNew)        { return dmlib_color::setNewColor(&DarkMode::getTheme().m_colors.background, clrNew); }
	COLORREF setCtrlBackgroundColor(COLORREF clrNew)    { return dmlib_color::setNewColor(&DarkMode::getTheme().m_colors.ctrlBackground, clrNew); }
	COLORREF setHotBackgroundColor(COLORREF clrNew)     { return dmlib_color::setNewColor(&DarkMode::getTheme().m_colors.hotBackground, clrNew); }
	COLORREF setDlgBackgroundColor(COLORREF clrNew)     { return dmlib_color::setNewColor(&DarkMode::getTheme().m_colors.dlgBackground, clrNew); }
	COLORREF setErrorBackgroundColor(COLORREF clrNew)   { return dmlib_color::setNewColor(&DarkMode::getTheme().m_colors.errorBackground, clrNew); }
	COLORREF setTextColor(COLORREF clrNew)              { return dmlib_color::setNewColor(&DarkMode::getTheme().m_colors.text, clrNew); }
	COLORREF setDarkerTextColor(COLORREF clrNew)        { return dmlib_color::setNewColor(&DarkMode::getTheme().m_colors.darkerText, clrNew); }
	COLORREF setDisabledTextColor(COLORREF clrNew)      { return dmlib_color::setNewColor(&DarkMode::getTheme().m_colors.disabledText, clrNew); }
	COLORREF setLinkTextColor(COLORREF clrNew)          { return dmlib_color::setNewColor(&DarkMode::getTheme().m_colors.linkText, clrNew); }
	COLORREF setEdgeColor(COLORREF clrNew)              { return dmlib_color::setNewColor(&DarkMode::getTheme().m_colors.edge, clrNew); }
	COLORREF setHotEdgeColor(COLORREF clrNew)           { return dmlib_color::setNewColor(&DarkMode::getTheme().m_colors.hotEdge, clrNew); }
	COLORREF setDisabledEdgeColor(COLORREF clrNew)      { return dmlib_color::setNewColor(&DarkMode::getTheme().m_colors.disabledEdge, clrNew); }

	void setThemeColors(Colors colors)
	{
		DarkMode::getTheme().updateTheme(colors);
	}

	void updateThemeBrushesAndPens()
	{
		DarkMode::getTheme().updateTheme();
	}

	COLORREF getBackgroundColor()         { return getTheme().m_colors.background; }
	COLORREF getCtrlBackgroundColor()     { return getTheme().m_colors.ctrlBackground; }
	COLORREF getHotBackgroundColor()      { return getTheme().m_colors.hotBackground; }
	COLORREF getDlgBackgroundColor()      { return getTheme().m_colors.dlgBackground; }
	COLORREF getErrorBackgroundColor()    { return getTheme().m_colors.errorBackground; }
	COLORREF getTextColor()               { return getTheme().m_colors.text; }
	COLORREF getDarkerTextColor()         { return getTheme().m_colors.darkerText; }
	COLORREF getDisabledTextColor()       { return getTheme().m_colors.disabledText; }
	COLORREF getLinkTextColor()           { return getTheme().m_colors.linkText; }
	COLORREF getEdgeColor()               { return getTheme().m_colors.edge; }
	COLORREF getHotEdgeColor()            { return getTheme().m_colors.hotEdge; }
	COLORREF getDisabledEdgeColor()       { return getTheme().m_colors.disabledEdge; }

	HBRUSH getBackgroundBrush()           { return getTheme().getBrushes().m_background; }
	HBRUSH getCtrlBackgroundBrush()       { return getTheme().getBrushes().m_ctrlBackground; }
	HBRUSH getHotBackgroundBrush()        { return getTheme().getBrushes().m_hotBackground; }
	HBRUSH getDlgBackgroundBrush()        { return getTheme().getBrushes().m_dlgBackground; }
	HBRUSH getErrorBackgroundBrush()      { return getTheme().getBrushes().m_errorBackground; }

	HBRUSH getEdgeBrush()                 { return getTheme().getBrushes().m_edge; }
	HBRUSH getHotEdgeBrush()              { return getTheme().getBrushes().m_hotEdge; }
	HBRUSH getDisabledEdgeBrush()         { return getTheme().getBrushes().m_disabledEdge; }

	HPEN getDarkerTextPen()               { return getTheme().getPens().m_darkerText; }
	HPEN getEdgePen()                     { return getTheme().getPens().m_edge; }
	HPEN getHotEdgePen()                  { return getTheme().getPens().m_hotEdge; }
	HPEN getDisabledEdgePen()             { return getTheme().getPens().m_disabledEdge; }

	COLORREF setViewBackgroundColor(COLORREF clrNew)        { return dmlib_color::setNewColor(&DarkMode::getThemeView().m_clrView.background, clrNew); }
	COLORREF setViewTextColor(COLORREF clrNew)              { return dmlib_color::setNewColor(&DarkMode::getThemeView().m_clrView.text, clrNew); }
	COLORREF setViewGridlinesColor(COLORREF clrNew)         { return dmlib_color::setNewColor(&DarkMode::getThemeView().m_clrView.gridlines, clrNew); }

	COLORREF setHeaderBackgroundColor(COLORREF clrNew)      { return dmlib_color::setNewColor(&DarkMode::getThemeView().m_clrView.headerBackground, clrNew); }
	COLORREF setHeaderHotBackgroundColor(COLORREF clrNew)   { return dmlib_color::setNewColor(&DarkMode::getThemeView().m_clrView.headerHotBackground, clrNew); }
	COLORREF setHeaderTextColor(COLORREF clrNew)            { return dmlib_color::setNewColor(&DarkMode::getThemeView().m_clrView.headerText, clrNew); }
	COLORREF setHeaderEdgeColor(COLORREF clrNew)            { return dmlib_color::setNewColor(&DarkMode::getThemeView().m_clrView.headerEdge, clrNew); }

	void setViewColors(ColorsView colors)
	{
		DarkMode::getThemeView().updateView(colors);
	}

	void updateViewBrushesAndPens()
	{
		DarkMode::getThemeView().updateView();
	}

	COLORREF getViewBackgroundColor()       { return DarkMode::getThemeView().m_clrView.background; }
	COLORREF getViewTextColor()             { return DarkMode::getThemeView().m_clrView.text; }
	COLORREF getViewGridlinesColor()        { return DarkMode::getThemeView().m_clrView.gridlines; }

	COLORREF getHeaderBackgroundColor()     { return DarkMode::getThemeView().m_clrView.headerBackground; }
	COLORREF getHeaderHotBackgroundColor()  { return DarkMode::getThemeView().m_clrView.headerHotBackground; }
	COLORREF getHeaderTextColor()           { return DarkMode::getThemeView().m_clrView.headerText; }
	COLORREF getHeaderEdgeColor()           { return DarkMode::getThemeView().m_clrView.headerEdge; }

	HBRUSH getViewBackgroundBrush()         { return DarkMode::getThemeView().getViewBrushesAndPens().m_background; }
	HBRUSH getViewGridlinesBrush()          { return DarkMode::getThemeView().getViewBrushesAndPens().m_gridlines; }

	HBRUSH getHeaderBackgroundBrush()       { return DarkMode::getThemeView().getViewBrushesAndPens().m_headerBackground; }
	HBRUSH getHeaderHotBackgroundBrush()    { return DarkMode::getThemeView().getViewBrushesAndPens().m_headerHotBackground; }

	HPEN getHeaderEdgePen()                 { return DarkMode::getThemeView().getViewBrushesAndPens().m_headerEdge; }

	/**
	 * @brief Initializes default color set based on the current mode type.
	 *
	 * Sets up control and view colors depending on the active theme:
	 * - `dark`: Applies dark tone color set and view dark color set.
	 * - `light`: Applies the predefined light color set and view light color set.
	 * - `classic`: Applies only system color on views, other controls are not affected
	 *              by theme colors.
	 *
	 * If `updateBrushesAndOther` is `true`, also updates
	 * brushes, pens, and view styles (unless in classic mode).
	 *
	 * @param updateBrushesAndOther Whether to refresh GDI brushes and pens, and tree view styling.
	 *
	 * @see DarkMode::setToneColors
	 * @see DarkMode::updateThemeBrushesAndPens
	 * @see DarkMode::calculateTreeViewStyle
	 */
	void setDefaultColors(bool updateBrushesAndOther)
	{
		switch (g_dmCfg.m_dmType)
		{
			case DarkModeType::dark:
			{
				DarkMode::getTheme().setToneColors();
				DarkMode::getThemeView().m_clrView = dmlib_color::kDarkColorsView;
				break;
			}

			case DarkModeType::light:
			{
				DarkMode::getTheme().m_colors = dmlib_color::getLightColors();
				DarkMode::getThemeView().m_clrView = dmlib_color::kLightColorsView;
				break;
			}

			case DarkModeType::classic:
			{
				DarkMode::setViewBackgroundColor(::GetSysColor(COLOR_WINDOW));
				DarkMode::setViewTextColor(::GetSysColor(COLOR_WINDOWTEXT));
				break;
			}
		}

		if (updateBrushesAndOther)
		{
			if (g_dmCfg.m_dmType != DarkModeType::classic)
			{
				DarkMode::updateThemeBrushesAndPens();
				DarkMode::updateViewBrushesAndPens();
			}

			DarkMode::calculateTreeViewStyle();
		}
	}

	/**
	 * @brief Initializes the dark mode configuration based on the selected mode.
	 *
	 * Sets the active dark mode theming and system-following behavior according to the specified `dmType`:
	 * - `0`: Light mode, do not follow system.
	 * - `1` or default: Dark mode, do not follow system.
	 * - `2`: *[Internal]* Follow system - light or dark depending on registry (see `DarkMode::isDarkModeReg()`).
	 * - `3`: Classic mode, do not follow system.
	 * - `4`: *[Internal]* Follow system - classic or dark depending on registry.
	 *
	 * @param dmType Integer representing the desired mode.
	 *
	 * @see DarkModeType
	 * @see WinMode
	 * @see DarkMode::isDarkModeReg()
	 */
	void initDarkModeConfig(UINT dmType)
	{
		switch (dmType)
		{
			case 0:
			{
				g_dmCfg.m_dmType = DarkModeType::light;
				g_dmCfg.m_windowsMode = WinMode::disabled;
				break;
			}

			case 2:
			{
				g_dmCfg.m_dmType = DarkMode::isDarkModeReg() ? DarkModeType::dark : DarkModeType::light;
				g_dmCfg.m_windowsMode = WinMode::light;
				break;
			}

			case 3:
			{
				g_dmCfg.m_dmType = DarkModeType::classic;
				g_dmCfg.m_windowsMode = WinMode::disabled;
				break;
			}

			case 4:
			{
				g_dmCfg.m_dmType = DarkMode::isDarkModeReg() ? DarkModeType::dark : DarkModeType::classic;
				g_dmCfg.m_windowsMode = WinMode::classic;
				break;
			}

			case 1:
			default:
			{
				g_dmCfg.m_dmType = DarkModeType::dark;
				g_dmCfg.m_windowsMode = WinMode::disabled;
				break;
			}
		}
	}

	/**
	 * @brief Sets the preferred window corner style on Windows 11.
	 *
	 * Assigns a valid `DWM_WINDOW_CORNER_PREFERENCE` value to the config,
	 * falling back to `DWMWCP_DEFAULT` if the input is out of range.
	 *
	 * @param roundCornerStyle Integer value representing a `DWM_WINDOW_CORNER_PREFERENCE`.
	 *
	 * @see https://learn.microsoft.com/windows/win32/api/dwmapi/ne-dwmapi-dwm_window_corner_preference
	 * @see DarkMode::setDarkTitleBarEx()
	 */
	void setRoundCornerConfig(UINT roundCornerStyle)
	{
		const auto cornerStyle = static_cast<DWM_WINDOW_CORNER_PREFERENCE>(roundCornerStyle);
		if (cornerStyle > DWMWCP_ROUNDSMALL) // || cornerStyle < DWMWCP_DEFAULT) // should never be < 0
		{
			g_dmCfg.m_roundCorner = DWMWCP_DEFAULT;
		}
		else
		{
			g_dmCfg.m_roundCorner = cornerStyle;
		}
	}

	static constexpr DWORD kDwmwaClrDefaultRGBCheck = 0x00FFFFFF;

	/**
	 * @brief Sets the preferred border color for window edge on Windows 11.
	 *
	 * Assigns the given `COLORREF` to the configuration. If the value matches
	 * `kDwmwaClrDefaultRGBCheck`, the color is reset to `DWMWA_COLOR_DEFAULT`.
	 *
	 * @param clr Border color value, or sentinel to reset to system default.
	 *
	 * @see DWMWA_BORDER_COLOR
	 * @see DarkMode::setDarkTitleBarEx()
	 */
	void setBorderColorConfig(COLORREF clr)
	{
		if (clr == kDwmwaClrDefaultRGBCheck)
		{
			g_dmCfg.m_borderColor = DWMWA_COLOR_DEFAULT;
		}
		else
		{
			g_dmCfg.m_borderColor = clr;
		}
	}

	/**
	 * @brief Sets the Mica effects on Windows 11 setting.
	 *
	 * Assigns a valid `DWM_SYSTEMBACKDROP_TYPE` to the configuration. If the value exceeds
	 * `DWMSBT_TABBEDWINDOW`, it falls back to `DWMSBT_AUTO`.
	 *
	 * @param mica Integer value representing a `DWM_SYSTEMBACKDROP_TYPE`.
	 *
	 * @see DWM_SYSTEMBACKDROP_TYPE
	 * @see DarkMode::setDarkTitleBarEx()
	 */
	void setMicaConfig(UINT mica)
	{
		const auto micaType = static_cast<DWM_SYSTEMBACKDROP_TYPE>(mica);
		if (micaType > DWMSBT_TABBEDWINDOW) // || micaType < DWMSBT_AUTO)  // should never be < 0
		{
			g_dmCfg.m_mica = DWMSBT_AUTO;
		}
		else
		{
			g_dmCfg.m_mica = micaType;
		}
	}

	/**
	 * @brief Sets Mica effects on the full window setting.
	 *
	 * Controls whether Mica should be applied to the entire window
	 * or limited to the title bar only.
	 *
	 * @param extendMica `true` to apply Mica to the full window, `false` for title bar only.
	 *
	 * @see DarkMode::setDarkTitleBarEx()
	 */
	void setMicaExtendedConfig(bool extendMica)
	{
		g_dmCfg.m_micaExtend = extendMica;
	}

	/**
	 * @brief Sets dialog colors on title bar on Windows 11 setting.
	 *
	 * Controls whether title bar should have same colors as dialog window.
	 *
	 * @param colorize `true` to have title bar to have same colors as dialog window.
	 *
	 * @see DarkMode::setDarkTitleBarEx()
	 */
	void setColorizeTitleBarConfig(bool colorize)
	{
		g_dmCfg.m_colorizeTitleBar = colorize;
	}

#if !defined(_DARKMODELIB_NO_INI_CONFIG)
	/**
	 * @brief Initializes dark mode configuration and colors from an INI file.
	 *
	 * Loads configuration values from the specified INI file path and applies them to the
	 * current dark mode settings. This includes:
	 * - Base appearance (`DarkModeType`) and system-following mode (`WinMode`)
	 * - Optional Mica and rounded corner styling
	 * - Custom colors for background, text, borders, and headers (if present)
	 * - Tone settings for dark theme (`ColorTone`)
	 *
	 * If the INI file does not exist, default dark mode behavior is applied via
	 * @ref DarkMode::setDarkModeConfigEx.
	 *
	 * @param iniName Name of INI file (resolved via @ref getIniPath).
	 *
	 * @note When `DarkModeType::classic` is set, system colors are used instead of themed ones.
	 */
	static void initOptions(const std::wstring& iniName)
	{
		if (iniName.empty())
		{
			return;
		}

		const auto iniPath = dmlib_ini::getIniPath(iniName);
		g_dmCfg.m_iniExist = dmlib_ini::fileExists(iniPath);
		if (g_dmCfg.m_iniExist)
		{
			DarkMode::initDarkModeConfig(::GetPrivateProfileIntW(L"main", L"mode", 1, iniPath.c_str()));
			if (g_dmCfg.m_dmType == DarkMode::DarkModeType::classic)
			{
				DarkMode::setDarkModeConfigEx(static_cast<UINT>(DarkMode::DarkModeType::classic));
				DarkMode::setDefaultColors(false);
				return;
			}

			const bool useDark = g_dmCfg.m_dmType == DarkMode::DarkModeType::dark;

			const std::wstring sectionBase = useDark ? L"dark" : L"light";
			const std::wstring sectionColorsView = sectionBase + L".colors.view";
			const std::wstring sectionColors = sectionBase + L".colors";

			DarkMode::setMicaConfig(::GetPrivateProfileIntW(sectionBase.c_str(), L"mica", 0, iniPath.c_str()));
			DarkMode::setRoundCornerConfig(::GetPrivateProfileIntW(sectionBase.c_str(), L"roundCorner", 0, iniPath.c_str()));
			dmlib_ini::setClrFromIni(iniPath, sectionBase, L"borderColor", &g_dmCfg.m_borderColor);
			if (g_dmCfg.m_borderColor == kDwmwaClrDefaultRGBCheck)
			{
				g_dmCfg.m_borderColor = DWMWA_COLOR_DEFAULT;
			}

			if (useDark)
			{
				UINT tone = ::GetPrivateProfileIntW(sectionBase.c_str(), L"tone", 0, iniPath.c_str());
				if (tone >= static_cast<UINT>(DarkMode::ColorTone::max))
				{
					tone = 0;
				}

				DarkMode::getTheme().setToneColors(static_cast<DarkMode::ColorTone>(tone));
				DarkMode::getThemeView().m_clrView = dmlib_color::kDarkColorsView;
				DarkMode::getThemeView().m_clrView.headerBackground = DarkMode::getTheme().m_colors.background;
				DarkMode::getThemeView().m_clrView.headerHotBackground = DarkMode::getTheme().m_colors.hotBackground;
				DarkMode::getThemeView().m_clrView.headerText = DarkMode::getTheme().m_colors.darkerText;

				if (!DarkMode::isWindowsModeEnabled())
				{
					g_dmCfg.m_micaExtend = (::GetPrivateProfileIntW(sectionBase.c_str(), L"micaExtend", 0, iniPath.c_str()) == 1);
				}
			}
			else
			{
				DarkMode::getTheme().m_colors = dmlib_color::getLightColors();
				DarkMode::getThemeView().m_clrView = dmlib_color::kLightColorsView;
			}

			struct ColorEntry
			{
				const wchar_t* key = nullptr;
				COLORREF* clr = nullptr;
			};

			static constexpr size_t nColorsViewMembers = 7;
			const std::array<ColorEntry, nColorsViewMembers> viewColors{ {
				{L"backgroundView", &DarkMode::getThemeView().m_clrView.background},
				{L"textView", &DarkMode::getThemeView().m_clrView.text},
				{L"gridlines", &DarkMode::getThemeView().m_clrView.gridlines},
				{L"backgroundHeader", &DarkMode::getThemeView().m_clrView.headerBackground},
				{L"backgroundHotHeader", &DarkMode::getThemeView().m_clrView.headerHotBackground},
				{L"textHeader", &DarkMode::getThemeView().m_clrView.headerText},
				{L"edgeHeader", &DarkMode::getThemeView().m_clrView.headerEdge}
			} };

			static constexpr size_t nColorsMembers = 12;
			const std::array<ColorEntry, nColorsMembers> baseColors{ {
				{L"background", &DarkMode::getTheme().m_colors.background},
				{L"backgroundCtrl", &DarkMode::getTheme().m_colors.ctrlBackground},
				{L"backgroundHot", &DarkMode::getTheme().m_colors.hotBackground},
				{L"backgroundDlg", &DarkMode::getTheme().m_colors.dlgBackground},
				{L"backgroundError", &DarkMode::getTheme().m_colors.errorBackground},
				{L"text", &DarkMode::getTheme().m_colors.text},
				{L"textItem", &DarkMode::getTheme().m_colors.darkerText},
				{L"textDisabled", &DarkMode::getTheme().m_colors.disabledText},
				{L"textLink", &DarkMode::getTheme().m_colors.linkText},
				{L"edge", &DarkMode::getTheme().m_colors.edge},
				{L"edgeHot", &DarkMode::getTheme().m_colors.hotEdge},
				{L"edgeDisabled", &DarkMode::getTheme().m_colors.disabledEdge}
			} };

			for (const auto& entry : viewColors)
			{
				dmlib_ini::setClrFromIni(iniPath, sectionColorsView, entry.key, entry.clr);
			}

			for (const auto& entry : baseColors)
			{
				dmlib_ini::setClrFromIni(iniPath, sectionColors, entry.key, entry.clr);
			}

			DarkMode::updateThemeBrushesAndPens();
			DarkMode::updateViewBrushesAndPens();
			DarkMode::calculateTreeViewStyle();

			if (!g_dmCfg.m_micaExtend)
			{
				g_dmCfg.m_colorizeTitleBar = (::GetPrivateProfileIntW(sectionBase.c_str(), L"colorizeTitleBar", 0, iniPath.c_str()) == 1);
			}

			dmlib_win32api::SetDarkMode(g_dmCfg.m_dmType == DarkMode::DarkModeType::dark, true);
		}
		else
		{
			DarkMode::setDarkModeConfigEx(static_cast<UINT>(DarkMode::DarkModeType::dark));
			DarkMode::setDefaultColors(true);
		}
	}
#endif // !defined(_DARKMODELIB_NO_INI_CONFIG)

	/**
	 * @brief Applies dark mode settings based on the given configuration type.
	 *
	 * Initializes the dark mode type settings and system-following behavior.
	 * Enables or disables dark mode depending on whether `DarkModeType::dark` is selected.
	 * It is recommended to use together with @ref DarkMode::setDefaultColors to also set colors.
	 *
	 * @param dmType Dark mode configuration type; see @ref DarkMode::initDarkModeConfig for values.
	 *
	 * @see DarkMode::setDarkModeConfig()
	 * @see DarkMode::initDarkModeConfig()
	 * @see DarkMode::setDefaultColors()
	 */
	void setDarkModeConfigEx(UINT dmType)
	{
		DarkMode::initDarkModeConfig(dmType);

		const bool useDark = g_dmCfg.m_dmType == DarkModeType::dark;
		dmlib_win32api::SetDarkMode(useDark, true);
	}

	/**
	 * @brief Applies dark mode settings based on system mode preference.
	 *
	 * Determines the appropriate mode using @ref DarkMode::isDarkModeReg and forwards
	 * the result to @ref DarkMode::setDarkModeConfigEx.
	 * It is recommended to use together with @ref DarkMode::setDefaultColors to also set colors.
	 *
	 * Uses:
	 * - `DarkModeType::dark` if registry prefers dark mode.
	 * - `DarkModeType::classic` otherwise.
	 *
	 * @see DarkMode::setDarkModeConfigEx()
	 */
	void setDarkModeConfig()
	{
		const auto dmType = static_cast<UINT>(DarkMode::isDarkModeReg() ? DarkModeType::dark : DarkModeType::classic);
		DarkMode::setDarkModeConfigEx(dmType);
	}

	/**
	 * @brief Initializes dark mode experimental features, colors, and other settings.
	 *
	 * Performs one-time setup for dark mode, including:
	 * - Initializing experimental features if not yet done.
	 * - Optionally loading settings from an INI file (if INI config is enabled).
	 * - Initializing TreeView style and applying dark mode settings.
	 * - Preparing system colors (e.g. `COLOR_WINDOW`, `COLOR_WINDOWTEXT`, `COLOR_BTNFACE`)
	 *   for hooking.
	 *
	 * @param iniName Optional path to an INI file for dark mode settings (ignored if already set).
	 *
	 * @note This function is only run once per session;
	 *       subsequent calls have no effect, unless follow system mode is used,
	 *       then only colors are updated each time system changes mode.
	 *
	 * @see DarkMode::initDarkMode()
	 * @see DarkMode::calculateTreeViewStyle()
	 */
	void initDarkModeEx([[maybe_unused]] const wchar_t* iniName)
	{
		if (!g_dmCfg.m_isInit)
		{
			if (!g_dmCfg.m_isInitExperimental)
			{
				dmlib_win32api::InitDarkMode();
				dmlib_dpi::InitDpiAPI();
				g_dmCfg.m_isInitExperimental = true;
			}

#if !defined(_DARKMODELIB_NO_INI_CONFIG)
			if (!g_dmCfg.m_isIniNameSet)
			{
				g_dmCfg.m_iniName = iniName;
				g_dmCfg.m_isIniNameSet = true;

				if (g_dmCfg.m_iniName.empty())
				{
					DarkMode::setDarkModeConfigEx(static_cast<UINT>(DarkModeType::dark));
					DarkMode::setDefaultColors(true);
				}
			}
			DarkMode::initOptions(g_dmCfg.m_iniName);
#else
			DarkMode::setDarkModeConfig();
			DarkMode::setDefaultColors(true);
#endif

			DarkMode::setSysColor(COLOR_WINDOW, DarkMode::getBackgroundColor());
			DarkMode::setSysColor(COLOR_WINDOWTEXT, DarkMode::getTextColor());
			DarkMode::setSysColor(COLOR_BTNFACE, DarkMode::getViewGridlinesColor());

			g_dmCfg.m_isInit = true;
		}
	}

	/**
	 * @brief Initializes dark mode without INI settings.
	 *
	 * Forwards to @ref DarkMode::initDarkModeEx with an empty INI path, effectively disabling INI settings.
	 *
	 * @see DarkMode::initDarkModeEx()
	 */
	void initDarkMode()
	{
		DarkMode::initDarkModeEx(L"");
	}

	/**
	 * @brief Checks if there is config INI file.
	 *
	 * @return `true` if there is config INI file that can be used.
	 */
	bool doesConfigFileExist()
	{
#if !defined(_DARKMODELIB_NO_INI_CONFIG)
		return g_dmCfg.m_iniExist;
#else
		return false;
#endif
	}

	/**
	 * @brief Checks if non-classic mode is enabled.
	 *
	 * If `_DARKMODELIB_ALLOW_OLD_OS` is defined with value larger than '1',
	 * this skips Windows version checks. Otherwise, dark mode is only enabled
	 * on Windows 10 or newer.
	 *
	 * @return `true` if a supported dark mode type is active, otherwise `false`.
	 */
	bool isEnabled()
	{
#if defined(_DARKMODELIB_ALLOW_OLD_OS) && (_DARKMODELIB_ALLOW_OLD_OS > 1)
		return g_dmCfg.m_dmType != DarkModeType::classic;
#else
		return DarkMode::isAtLeastWindows10() && g_dmCfg.m_dmType != DarkModeType::classic;
#endif
	}

	/**
	 * @brief Checks if experimental dark mode features are currently active.
	 *
	 * @return `true` if experimental dark mode is enabled.
	 */
	bool isExperimentalActive()
	{
		return g_darkModeEnabled;
	}

	/**
	 * @brief Checks if experimental dark mode features are supported by the system.
	 *
	 * @return `true` if dark mode experimental APIs are available.
	 */
	bool isExperimentalSupported()
	{
		return g_darkModeSupported;
	}

	/**
	 * @brief Checks if follow the system mode behavior is enabled.
	 *
	 * @return `true` if "mode" is not `WinMode::disabled`, i.e. system mode is followed.
	 */
	bool isWindowsModeEnabled()
	{
		return g_dmCfg.m_windowsMode != WinMode::disabled;
	}

	/**
	 * @brief Checks if the host OS is at least Windows 10.
	 *
	 * @return `true` if running on Windows 10 or newer.
	 */
	bool isAtLeastWindows10()
	{
		return dmlib_win32api::IsWindows10();
	}
	/**
	 * @brief Checks if the host OS is at least Windows 11.
	 *
	 * @return `true` if running on Windows 11 or newer.
	 */
	bool isAtLeastWindows11()
	{
		return dmlib_win32api::IsWindows11();
	}

	/**
	 * @brief Retrieves the current Windows build number.
	 *
	 * @return Windows build number reported by the system.
	 */
	DWORD getWindowsBuildNumber()
	{
		return dmlib_win32api::GetWindowsBuildNumber();
	}

	/**
	 * @brief Handles system setting changes related to dark mode.
	 *
	 * Responds to system messages indicating a color scheme change. If the current
	 * dark mode state no longer matches the system registry preference, dark mode is
	 * re-initialized.
	 *
	 * - Skips processing if experimental dark mode is unsupported.
	 * - Relies on @ref DarkMode::isDarkModeReg for theme preference and skips during high contrast.
	 *
	 * @param lParam Message parameter (typically from `WM_SETTINGCHANGE`).
	 * @return `true` if a dark mode change was handled; otherwise `false`.
	 *
	 * @see DarkMode::isDarkModeReg()
	 * @see DarkMode::initDarkMode()
	 */
	bool handleSettingChange(LPARAM lParam)
	{
		if (DarkMode::isExperimentalSupported()
			&& dmlib_win32api::IsColorSchemeChangeMessage(lParam))
		{
			// fnShouldAppsUseDarkMode (ordinal 132) is not reliable on 1903+, use DarkMode::isDarkModeReg() instead
			const bool isDarkModeUsed = (DarkMode::isDarkModeReg() && !dmlib_win32api::IsHighContrast());
			if (DarkMode::isExperimentalActive() != isDarkModeUsed)
			{
				if (g_dmCfg.m_isInit)
				{
					g_dmCfg.m_isInit = false;
					DarkMode::initDarkMode();
				}
			}
			return true;
		}
		return false;
	}

	/**
	 * @brief Checks if dark mode is enabled in the Windows registry.
	 *
	 * Queries `HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize\\AppsUseLightTheme`.
	 *
	 * @return `true` if dark mode is preferred (value is `0`); otherwise `false`.
	 */
	bool isDarkModeReg()
	{
		DWORD data{};
		DWORD dwBufSize = sizeof(data);
		LPCWSTR lpSubKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
		LPCWSTR lpValue = L"AppsUseLightTheme";

		if (::RegGetValueW(HKEY_CURRENT_USER, lpSubKey, lpValue, RRF_RT_REG_DWORD, nullptr, &data, &dwBufSize) != ERROR_SUCCESS)
		{
			return false;
		}

		// dark mode is 0, light mode is 1
		return data == 0UL;
	}

	/**
	 * @brief Overrides a specific system color with a custom color.
	 *
	 * Currently supports:
	 * - `COLOR_WINDOW`: Background of ComboBoxEx list.
	 * - `COLOR_WINDOWTEXT`: Text color of ComboBoxEx list.
	 * - `COLOR_BTNFACE`: Gridline color in ListView (when applicable).
	 *
	 * @param[in]   nIndex  One of the supported system color indices.
	 * @param[in]   color   Custom `COLORREF` value to apply.
	 */
	void setSysColor(int nIndex, COLORREF color)
	{
		dmlib_hook::setMySysColor(nIndex, color);
	}

	/**
	 * @brief Makes scroll bars on the specified window and all its children consistent.
	 *
	 * @note Currently not widely used by default.
	 *
	 * @param hWnd Handle to the parent window.
	 */
	void enableDarkScrollBarForWindowAndChildren([[maybe_unused]] HWND hWnd)
	{
#if defined(_DARKMODELIB_USE_SCROLLBAR_FIX) && (_DARKMODELIB_USE_SCROLLBAR_FIX > 0)
		dmlib_hook::enableDarkScrollBarForWindowAndChildren(hWnd);
#endif
	}

	/**
	 * @brief Checks if current mode is dark type.
	 */
	bool isDarkDmTypeUsed()
	{
		return g_dmCfg.m_dmType == DarkMode::DarkModeType::dark;
	}

	/**
	 * @brief Applies themed owner drawn subclassing to a checkbox, radio, or tri-state button control.
	 *
	 * Associates a `ButtonData` instance with the control.
	 *
	 * @param hWnd Handle to the checkbox, radio, or tri-state button control.
	 *
	 * @see dmlib_subclass::ButtonSubclass()
	 * @see DarkMode::removeCheckboxOrRadioBtnCtrlSubclass()
	 */
	void setCheckboxOrRadioBtnCtrlSubclass(HWND hWnd)
	{
		dmlib_subclass::SetSubclass<dmlib_subclass::ButtonData>(hWnd, dmlib_subclass::ButtonSubclass, dmlib_subclass::SubclassID::button, hWnd);
	}

	/**
	 * @brief Removes the owner drawn subclass from a checkbox, radio, or tri-state button control.
	 *
	 * Cleans up the `ButtonData` instance and detaches the control's subclass proc.
	 *
	 * @param hWnd Handle to the previously subclassed control.
	 *
	 * @see dmlib_subclass::ButtonSubclass()
	 * @see DarkMode::setCheckboxOrRadioBtnCtrlSubclass()
	 */
	void removeCheckboxOrRadioBtnCtrlSubclass(HWND hWnd)
	{
		dmlib_subclass::RemoveSubclass<dmlib_subclass::ButtonData>(hWnd, dmlib_subclass::ButtonSubclass, dmlib_subclass::SubclassID::button);
	}

	/**
	 * @brief Applies owner drawn subclassing to a groupbox button control.
	 *
	 * Associates a `ButtonData` instance with the control.
	 *
	 * @param hWnd Handle to the groupbox button control.
	 *
	 * @see dmlib_subclass::GroupboxSubclass()
	 * @see DarkMode::removeGroupboxCtrlSubclass()
	 */
	void setGroupboxCtrlSubclass(HWND hWnd)
	{
		dmlib_subclass::SetSubclass<dmlib_subclass::ButtonData>(hWnd, dmlib_subclass::GroupboxSubclass, dmlib_subclass::SubclassID::groupbox);
	}

	/**
	 * @brief Removes the owner drawn subclass from a groupbox button control.
	 *
	 * Cleans up the `ButtonData` instance and detaches the control's subclass proc.
	 *
	 * @param hWnd Handle to the previously subclassed control.
	 *
	 * @see dmlib_subclass::GroupboxSubclass()
	 * @see DarkMode::setGroupboxCtrlSubclass()
	 */
	void removeGroupboxCtrlSubclass(HWND hWnd)
	{
		dmlib_subclass::RemoveSubclass<dmlib_subclass::ButtonData>(hWnd, dmlib_subclass::GroupboxSubclass, dmlib_subclass::SubclassID::groupbox);
	}

	/**
	 * @brief Applies theming and/or subclassing to a button control based on its style.
	 *
	 * Inspects the control's style (`BS_*`) to determine its visual category and applies
	 * apropriate theming and/or subclassing accordingly. Handles:
	 * - Checkbox/radio/tri-state buttons: Applies theme (optional) and optional subclassing
	 * - Group boxes: Applies subclassing for dark mode drawing
	 * - Push buttons: Applies visual theming if requested
	 *
	 * The behavior varies depending on dark mode support, Windows version, and the flags
	 * provided in @ref DarkModeParams.
	 *
	 * @param hWnd  Handle to the target button control.
	 * @param p     Parameters defining theming and subclassing behavior.
	 *
	 * @see DarkModeParams
	 * @see DarkMode::setCheckboxOrRadioBtnCtrlSubclass()
	 * @see DarkMode::setGroupboxCtrlSubclass()
	 */
	static void setBtnCtrlSubclassAndTheme(HWND hWnd, DarkModeParams p)
	{
		const auto nBtnStyle = ::GetWindowLongPtr(hWnd, GWL_STYLE);
		switch (nBtnStyle & BS_TYPEMASK)
		{
			case BS_CHECKBOX:
			case BS_AUTOCHECKBOX:
			case BS_3STATE:
			case BS_AUTO3STATE:
			case BS_RADIOBUTTON:
			case BS_AUTORADIOBUTTON:
			{
				if ((nBtnStyle & BS_PUSHLIKE) == BS_PUSHLIKE)
				{
					if (p.m_theme)
					{
						::SetWindowTheme(hWnd, p.m_themeClassName, nullptr);
					}
					break;
				}

				if (DarkMode::isAtLeastWindows11() && p.m_theme)
				{
					::SetWindowTheme(hWnd, p.m_themeClassName, nullptr);
				}

				if (p.m_subclass)
				{
					DarkMode::setCheckboxOrRadioBtnCtrlSubclass(hWnd);
				}
				break;
			}

			case BS_GROUPBOX:
			{
				if (p.m_subclass)
				{
					DarkMode::setGroupboxCtrlSubclass(hWnd);
				}
				break;
			}

			case BS_PUSHBUTTON:
			case BS_DEFPUSHBUTTON:
			case BS_SPLITBUTTON:
			case BS_DEFSPLITBUTTON:
			{
				if (p.m_theme)
				{
					::SetWindowTheme(hWnd, p.m_themeClassName, nullptr);
				}
				break;
			}

			default:
			{
				break;
			}
		}
	}

	/**
	 * @brief Applies owner drawn subclassing and theming to an up-down (spinner) control.
	 *
	 * Associates a `UpDownData` instance with the control.
	 *
	 * @param hWnd Handle to the up-down (spinner) control.
	 *
	 * @see dmlib_subclass::UpDownSubclass()
	 * @see DarkMode::removeUpDownCtrlSubclass()
	 */
	void setUpDownCtrlSubclass(HWND hWnd)
	{
		dmlib_subclass::SetSubclass<dmlib_subclass::UpDownData>(hWnd, dmlib_subclass::UpDownSubclass, dmlib_subclass::SubclassID::upDown, hWnd);
		DarkMode::setDarkExplorerTheme(hWnd);
	}

	/**
	 * @brief Removes the owner drawn subclass from a up-down (spinner) control.
	 *
	 * Cleans up the `UpDownData` instance and detaches the control's subclass proc.
	 *
	 * @param hWnd Handle to the previously subclassed control.
	 *
	 * @see dmlib_subclass::UpDownSubclass()
	 * @see DarkMode::setUpDownCtrlSubclass()
	 */
	void removeUpDownCtrlSubclass(HWND hWnd)
	{
		dmlib_subclass::RemoveSubclass<dmlib_subclass::UpDownData>(hWnd, dmlib_subclass::UpDownSubclass, dmlib_subclass::SubclassID::upDown);
	}

	/**
	 * @brief Applies up-down (spinner) control theming and/or subclassing based on specified parameters.
	 *
	 * Conditionally applies custom subclassing and/or themed appearance
	 * depending on `DarkModeParams`. Subclassing takes priority if both are requested.
	 *
	 * @param hWnd  Handle to the up-down control.
	 * @param p     Parameters controlling whether to apply theming and/or subclassing.
	 *
	 * @see DarkModeParams
	 * @see DarkMode::setUpDownCtrlSubclass()
	 */
	static void setUpDownCtrlSubclassAndTheme(HWND hWnd, DarkModeParams p)
	{
		if (p.m_subclass)
		{
			DarkMode::setUpDownCtrlSubclass(hWnd);
		}
		else if (p.m_theme)
		{
			::SetWindowTheme(hWnd, p.m_themeClassName, nullptr);
		}
	}

	/**
	 * @brief Applies owner drawn subclassing to a tab control.
	 *
	 * @param hWnd Handle to the tab control.
	 *
	 * @see dmlib_subclass::TabPaintSubclass()
	 * @see DarkMode::removeTabCtrlPaintSubclass()
	 */
	static void setTabCtrlPaintSubclass(HWND hWnd)
	{
		dmlib_subclass::SetSubclass<dmlib_subclass::TabData>(hWnd, dmlib_subclass::TabPaintSubclass, dmlib_subclass::SubclassID::tabPaint);
	}

	/**
	 * @brief Removes the owner drawn subclass from a tab control.
	 *
	 * Cleans up the `TabData` instance and detaches the control's subclass proc.
	 *
	 * @param hWnd Handle to the previously subclassed tab control.
	 *
	 * @see dmlib_subclass::TabPaintSubclass()
	 * @see DarkMode::setTabCtrlPaintSubclass()
	 */
	static void removeTabCtrlPaintSubclass(HWND hWnd)
	{
		dmlib_subclass::RemoveSubclass<dmlib_subclass::TabData>(hWnd, dmlib_subclass::TabPaintSubclass, dmlib_subclass::SubclassID::tabPaint);
	}

	/**
	 * @brief Applies a subclass to detect and subclass tab control's up-down (spinner) child.
	 *
	 * Enable automatic subclassing of the up-down (spinner) control
	 * when it's created dynamically (for `TCS_SCROLLOPPOSITE` or overflow).
	 *
	 * @param hWnd Handle to the tab control.
	 *
	 * @see dmlib_subclass::TabUpDownSubclass()
	 * @see DarkMode::removeTabCtrlUpDownSubclass()
	 */
	void setTabCtrlUpDownSubclass(HWND hWnd)
	{
		dmlib_subclass::SetSubclass(hWnd, dmlib_subclass::TabUpDownSubclass, dmlib_subclass::SubclassID::tabUpDown);
	}

	/**
	 * @brief Removes the subclass procedure for a tab control's up-down (spinner) child detection.
	 *
	 * Detaches the control's subclass proc.
	 *
	 * @param hWnd Handle to the previously subclassed tab control.
	 *
	 * @see dmlib_subclass::TabUpDownSubclass()
	 * @see DarkMode::setTabCtrlUpDownSubclass()
	 */
	void removeTabCtrlUpDownSubclass(HWND hWnd)
	{
		dmlib_subclass::RemoveSubclass(hWnd, dmlib_subclass::TabUpDownSubclass, dmlib_subclass::SubclassID::tabUpDown);
	}

	/**
	 * @brief Applies owner drawn and up-down (spinner) child detection subclassings for a tab control.
	 *
	 * Applies both @ref DarkMode::TabPaintSubclass() for custom drawing
	 * and @ref DarkMode::TabUpDownSubclass() for detecting and subclassing
	 * the associated up-down (spinner) control.
	 *
	 * @param hWnd Handle to the tab control.
	 *
	 * @see DarkMode::removeTabCtrlSubclass()
	 * @see DarkMode::setTabCtrlPaintSubclass()
	 * @see DarkMode::setTabCtrlUpDownSubclass()
	 */
	void setTabCtrlSubclass(HWND hWnd)
	{
		DarkMode::setTabCtrlPaintSubclass(hWnd);
		DarkMode::setTabCtrlUpDownSubclass(hWnd);
	}

	/**
	 * @brief Removes owner drawn and up-down (spinner) child detection subclasses.
	 *
	 * Detaches the control's subclass procs.
	 *
	 * @param hWnd Handle to the previously subclassed tab control.
	 *
	 * @see DarkMode::setTabCtrlSubclass()
	 * @see DarkMode::removeTabCtrlPaintSubclass()
	 * @see DarkMode::removeTabCtrlUpDownSubclass()
	 */
	void removeTabCtrlSubclass(HWND hWnd)
	{
		DarkMode::removeTabCtrlPaintSubclass(hWnd);
		DarkMode::removeTabCtrlUpDownSubclass(hWnd);
	}

	/**
	 * @brief Applies tab control theming and subclassing based on specified parameters.
	 *
	 * Conditionally applies tooltip theming and tab control subclassing
	 * depending on `DarkModeParams`.
	 *
	 * @param hWnd  Handle to the tab control.
	 * @param p     Parameters controlling whether to apply theming and/or subclassing.
	 *
	 * @see DarkModeParams
	 * @see DarkMode::setDarkTooltips()
	 * @see DarkMode::setTabCtrlSubclass()
	 */
	static void setTabCtrlSubclassAndTheme(HWND hWnd, DarkModeParams p)
	{
		if (p.m_theme)
		{
			DarkMode::setDarkTooltips(hWnd, static_cast<int>(ToolTipsType::tabbar));
		}

		if (p.m_subclass)
		{
			DarkMode::setTabCtrlSubclass(hWnd);
		}
	}

	/**
	 * @brief Applies owner drawn custom border subclassing to a list box or edit control.
	 *
	 * @param hWnd Handle to the list box or edit control.
	 *
	 * @see dmlib_subclass::CustomBorderSubclass()
	 * @see DarkMode::removeCustomBorderForListBoxOrEditCtrlSubclass()
	 */
	void setCustomBorderForListBoxOrEditCtrlSubclass(HWND hWnd)
	{
		dmlib_subclass::SetSubclass<dmlib_subclass::BorderMetricsData>(hWnd, dmlib_subclass::CustomBorderSubclass, dmlib_subclass::SubclassID::customBorder, hWnd);
	}

	/**
	 * @brief Removes the custom border subclass from a list box or edit control.
	 *
	 * Cleans up the `BorderMetricsData` and detaches the control's subclass proc,
	 * restoring the control's default border drawing.
	 *
	 * @param hWnd Handle to the previously subclassed control.
	 *
	 * @see dmlib_subclass::CustomBorderSubclass()
	 * @see DarkMode::setCustomBorderForListBoxOrEditCtrlSubclass()
	 */
	void removeCustomBorderForListBoxOrEditCtrlSubclass(HWND hWnd)
	{
		dmlib_subclass::RemoveSubclass<dmlib_subclass::BorderMetricsData>(hWnd, dmlib_subclass::CustomBorderSubclass, dmlib_subclass::SubclassID::customBorder);
	}

	/**
	 * @brief Applies theming and optional custom border subclassing to a list box or edit control.
	 *
	 * Conditionally configures the visual style of a list box or edit control
	 * depending on `DarkModeParams`, control type, and window styles.
	 * Applies a custom border subclass for controls with `WS_EX_CLIENTEDGE` flag.
	 * Toggle the client edge style depending on dark mode state.
	 *
	 * @param hWnd      Handle to the target list box or edit control.
	 * @param p         Parameters controlling whether to apply theming and/or subclassing.
	 * @param isListBox `true` if the control is a list box, `false` if it's an edit control.
	 *
	 * @note Custom border subclassing is skipped for combo box list boxes.
	 *
	 * @see DarkModeParams
	 * @see DarkMode::setCustomBorderForListBoxOrEditCtrlSubclass()
	 */
	static void setCustomBorderForListBoxOrEditCtrlSubclassAndTheme(HWND hWnd, DarkModeParams p, bool isListBox)
	{
		const auto nStyle = ::GetWindowLongPtr(hWnd, GWL_STYLE);
		const bool hasScrollBar = ((nStyle & WS_HSCROLL) == WS_HSCROLL) || ((nStyle & WS_VSCROLL) == WS_VSCROLL);

		// edit control without scroll bars
		if (dmlib_subclass::isThemePrefered()
			&& p.m_theme
			&& !isListBox
			&& !hasScrollBar)
		{
			DarkMode::setDarkThemeExperimentalEx(hWnd, L"CFD");
		}
		else
		{
			if (p.m_theme && (isListBox || hasScrollBar))
			{
				// dark scroll bars for list box or edit control
				::SetWindowTheme(hWnd, p.m_themeClassName, nullptr);
			}

			const auto nExStyle = ::GetWindowLongPtr(hWnd, GWL_EXSTYLE);
			const bool hasClientEdge = (nExStyle & WS_EX_CLIENTEDGE) == WS_EX_CLIENTEDGE;
			const bool isCBoxListBox = isListBox && (nStyle & LBS_COMBOBOX) == LBS_COMBOBOX;

			if (p.m_subclass && hasClientEdge && !isCBoxListBox)
			{
				DarkMode::setCustomBorderForListBoxOrEditCtrlSubclass(hWnd);
			}

			if (::GetWindowSubclass(hWnd, dmlib_subclass::CustomBorderSubclass, static_cast<UINT_PTR>(dmlib_subclass::SubclassID::customBorder), nullptr) == TRUE)
			{
				const bool enableClientEdge = !DarkMode::isEnabled();
				DarkMode::setWindowExStyle(hWnd, enableClientEdge, WS_EX_CLIENTEDGE);
			}
		}
	}

	/**
	 * @brief Applies owner drawn subclassing to a combo box control.
	 *
	 * Retrieves the combo box style from the window and passes it to the subclass data
	 * (`ComboBoxData`) so the paint routine can adapt to that combo box style.
	 *
	 * @param hWnd Handle to the combo box control.
	 *
	 * @note Uses `GetWindowLongPtr` to extract the style bits.
	 *
	 * @see dmlib_subclass::ComboBoxSubclass()
	 * @see DarkMode::removeComboBoxCtrlSubclass()
	 */
	void setComboBoxCtrlSubclass(HWND hWnd)
	{
		const auto cbStyle = ::GetWindowLongPtr(hWnd, GWL_STYLE) & CBS_DROPDOWNLIST;
		dmlib_subclass::SetSubclass<dmlib_subclass::ComboBoxData>(hWnd, dmlib_subclass::ComboBoxSubclass, dmlib_subclass::SubclassID::comboBox, cbStyle);
	}

	/**
	 * @brief Removes the owner drawn subclass from a combo box control.
	 *
	 * Cleans up the `ComboBoxData` and detaches the control's subclass proc.
	 *
	 * @param hWnd Handle to the combo box control.
	 *
	 * @see dmlib_subclass::ComboBoxSubclass()
	 * @see DarkMode::setComboBoxCtrlSubclass()
	 */
	void removeComboBoxCtrlSubclass(HWND hWnd)
	{
		dmlib_subclass::RemoveSubclass<dmlib_subclass::ComboBoxData>(hWnd, dmlib_subclass::ComboBoxSubclass, dmlib_subclass::SubclassID::comboBox);
	}

	/**
	 * @brief Applies theming and optional subclassing to a combo box control.
	 *
	 * Configures a combo box' appearance and behavior based on its style,
	 * the provided parameters, and current theme preferences.
	 *
	 * Behavior:
	 * - If theming is enabled (`p.m_theme`) and the combo box has an associated list box:
	 *   - For `CBS_SIMPLE`, replaces the client edge with a custom border for non-classic mode.
	 *   - Applies themed scroll bars.
	 * - If subclassing is enabled (`p.m_subclass`) and dark mode is not the preferred theme:
	 *   - Applies a combo box subclassing unless the parent is a `ComboBoxEx` control.
	 * - If theming is enabled (`p.m_theme`):
	 *   - Applies the experimental `"CFD"` dark theme to the combo box for a light drop-down arrow.
	 *   - Clears the edit selection for non-`CBS_DROPDOWNLIST` styles to avoid visual artifacts.
	 *
	 * @param hWnd  Handle to the combo box control.
	 * @param p     Parameters controlling whether to apply theming and/or subclassing.
	 *
	 * @note Skips subclassing for `ComboBoxEx` parents to avoid conflicts.
	 *
	 * @see DarkModeParams
	 * @see DarkMode::setComboBoxCtrlSubclass()
	 */
	static void setComboBoxCtrlSubclassAndTheme(HWND hWnd, DarkModeParams p)
	{
		const auto cbStyle = ::GetWindowLongPtr(hWnd, GWL_STYLE) & CBS_DROPDOWNLIST;
		const bool isCbList = cbStyle == CBS_DROPDOWNLIST;
		const bool isCbSimple = cbStyle == CBS_SIMPLE;

		if (isCbList
			|| cbStyle == CBS_DROPDOWN
			|| isCbSimple)
		{
			COMBOBOXINFO cbi{};
			cbi.cbSize = sizeof(COMBOBOXINFO);
			if (::GetComboBoxInfo(hWnd, &cbi) == TRUE)
			{
				if (p.m_theme && cbi.hwndList != nullptr)
				{
					if (isCbSimple)
					{
						DarkMode::replaceClientEdgeWithBorderSafe(cbi.hwndList);
					}

					// dark scroll bar for list box of combo box
					::SetWindowTheme(cbi.hwndList, p.m_themeClassName, nullptr);
				}
			}

			if (!dmlib_subclass::isThemePrefered() && p.m_subclass)
			{
				HWND hParent = ::GetParent(hWnd);
				if ((hParent == nullptr || dmlib_subclass::getWndClassName(hParent) != WC_COMBOBOXEX))
				{
					DarkMode::setComboBoxCtrlSubclass(hWnd);
				}
			}

			if (p.m_theme) // for light dropdown arrow in dark mode
			{
				DarkMode::setDarkThemeExperimentalEx(hWnd, L"CFD");

				if (!isCbList)
				{
					::SendMessage(hWnd, CB_SETEDITSEL, 0, 0); // clear selection
				}
			}
		}
	}

	/**
	 * @brief Applies subclassing to a ComboBoxEx control to handle its child list box and edit controls.
	 *
	 * @param hWnd Handle to the ComboBoxEx control.
	 *
	 * @note Uses IAT hooking for custom colors.
	 *
	 * @see dmlib_subclass::ComboBoxSubclass()
	 * @see DarkMode::removeComboBoxExCtrlSubclass()
	 */
	void setComboBoxExCtrlSubclass(HWND hWnd)
	{
		dmlib_subclass::SetSubclass(hWnd, dmlib_subclass::ComboBoxExSubclass, dmlib_subclass::SubclassID::comboBoxEx);
	}

	/**
	 * @brief Removes the child handling subclass from a ComboBoxEx control.
	 *
	 * Detaches the control's subclass proc and unhooks system color changes.
	 *
	 * @param hWnd Handle to the ComboBoxEx control.
	 *
	 * @see dmlib_subclass::ComboBoxSubclass()
	 * @see DarkMode::setComboBoxExCtrlSubclass()
	 */
	void removeComboBoxExCtrlSubclass(HWND hWnd)
	{
		dmlib_subclass::RemoveSubclass(hWnd, dmlib_subclass::ComboBoxExSubclass, dmlib_subclass::SubclassID::comboBoxEx);
		dmlib_hook::unhookSysColor();
	}

	/**
	 * @brief Applies subclassing to a ComboBoxEx control to handle its child list box and edit controls.
	 *
	 * Overload wrapper that applies the subclass only if `p.m_subclass` is `true`.
	 *
	 * @param hWnd  Handle to the ComboBoxEx control.
	 * @param p     Parameters controlling whether to apply subclassing.
	 *
	 * @see DarkMode::setComboBoxExCtrlSubclass()
	 */
	static void setComboBoxExCtrlSubclass(HWND hWnd, DarkModeParams p)
	{
		if (p.m_subclass)
		{
			DarkMode::setComboBoxExCtrlSubclass(hWnd);
		}
	}

	/**
	 * @brief Applies subclassing to a list view control to handle custom colors.
	 *
	 * Handles custom colors for gridlines, header text, and in-place edit controls.
	 *
	 * @param hWnd Handle to the list view control.
	 *
	 * @note Uses IAT hooking for gridlines colors.
	 *
	 * @see dmlib_subclass::ListViewSubclass()
	 * @see DarkMode::removeListViewCtrlSubclass()
	 */
	void setListViewCtrlSubclass(HWND hWnd)
	{
		dmlib_subclass::SetSubclass(hWnd, dmlib_subclass::ListViewSubclass, dmlib_subclass::SubclassID::listView);
	}

	/**
	 * @brief Removes the custom colors handling subclass from a list view control.
	 *
	 * Detaches the control's subclass proc.
	 *
	 * @param hWnd Handle to the list view control.
	 *
	 * @see dmlib_subclass::ListViewSubclass()
	 * @see DarkMode::setListViewCtrlSubclass()
	 */
	void removeListViewCtrlSubclass(HWND hWnd)
	{
		dmlib_subclass::RemoveSubclass(hWnd, dmlib_subclass::ListViewSubclass, dmlib_subclass::SubclassID::listView);
	}

	/**
	 * @brief Applies theming and optional subclassing to a list view control.
	 *
	 * Configures colors, header theming, checkbox styling, and tooltips theming.
	 * Optionally installs the list view and header control subclasses for custom drawing.
	 * Enables double-buffering via `LVS_EX_DOUBLEBUFFER` flag.
	 *
	 * @param hWnd  Handle to the list view control.
	 * @param p     Parameters controlling whether to apply theming and/or subclassing.
	 *
	 * @see DarkMode::setDarkListView()
	 * @see DarkMode::setDarkListViewCheckboxes()
	 * @see DarkMode::setDarkTooltips()
	 * @see DarkMode::setListViewCtrlSubclass()
	 * @see DarkMode::setHeaderCtrlSubclass()
	 */
	static void setListViewCtrlSubclassAndTheme(HWND hWnd, DarkModeParams p)
	{
		HWND hHeader = ListView_GetHeader(hWnd);

		if (p.m_theme)
		{
			ListView_SetTextColor(hWnd, DarkMode::getViewTextColor());
			ListView_SetTextBkColor(hWnd, DarkMode::getViewBackgroundColor());
			ListView_SetBkColor(hWnd, DarkMode::getViewBackgroundColor());

			DarkMode::setDarkListView(hWnd);
			DarkMode::setDarkListViewCheckboxes(hWnd);
			DarkMode::setDarkTooltips(hWnd, static_cast<int>(ToolTipsType::listview));

			if (dmlib_subclass::isThemePrefered())
			{
				DarkMode::setDarkThemeExperimentalEx(hHeader, L"ItemsView");
			}
		}

		if (p.m_subclass)
		{
			if (!dmlib_subclass::isThemePrefered())
			{
				DarkMode::setHeaderCtrlSubclass(hHeader);
			}

			const auto lvExStyle = ListView_GetExtendedListViewStyle(hWnd);
			ListView_SetExtendedListViewStyle(hWnd, lvExStyle | LVS_EX_DOUBLEBUFFER);
			DarkMode::setListViewCtrlSubclass(hWnd);
		}
	}

	/**
	 * @brief Applies owner drawn subclassing to a header control.
	 *
	 * Retrieves the header button style from the window and passes it to the subclass data
	 * (`HeaderData`) so the paint routine can adapt to that header style.
	 *
	 * @param hWnd Handle to the header control.
	 *
	 * @note Uses `GetWindowLongPtr` to extract the style bits.
	 *
	 * @see dmlib_subclass::HeaderSubclass()
	 * @see DarkMode::removeHeaderCtrlSubclass()
	 */
	void setHeaderCtrlSubclass(HWND hWnd)
	{
		const bool hasBtnStyle = (::GetWindowLongPtr(hWnd, GWL_STYLE) & HDS_BUTTONS) == HDS_BUTTONS;
		dmlib_subclass::SetSubclass<dmlib_subclass::HeaderData>(hWnd, dmlib_subclass::HeaderSubclass, dmlib_subclass::SubclassID::header, hasBtnStyle);
	}

	/**
	 * @brief Removes the owner drawn subclass from a header control.
	 *
	 * Cleans up the `HeaderData` and detaches the control's subclass proc.
	 *
	 * @param hWnd Handle to the header control.
	 *
	 * @see dmlib_subclass::HeaderSubclass()
	 * @see DarkMode::setHeaderCtrlSubclass()
	 */
	void removeHeaderCtrlSubclass(HWND hWnd)
	{
		dmlib_subclass::RemoveSubclass<dmlib_subclass::HeaderData>(hWnd, dmlib_subclass::HeaderSubclass, dmlib_subclass::SubclassID::header);
	}

	/**
	 * @brief Applies owner drawn subclassing to a status bar control.
	 *
	 * Retrieves the status bar system font and passes it to the subclass data
	 * (`StatusBarData`).
	 *
	 * @param hWnd Handle to the status bar control.
	 *
	 * @note Uses `SystemParametersInfoW` to extract the `lfStatusFont` font.
	 *
	 * @see dmlib_subclass::StatusBarSubclass()
	 * @see DarkMode::removeStatusBarCtrlSubclass()
	 */
	void setStatusBarCtrlSubclass(HWND hWnd)
	{
		LOGFONT lf{};
		NONCLIENTMETRICS ncm{};
		ncm.cbSize = sizeof(NONCLIENTMETRICS);
		if (::SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0) != FALSE)
		{
			lf = ncm.lfStatusFont;
		}
		dmlib_subclass::SetSubclass<dmlib_subclass::StatusBarData>(hWnd, dmlib_subclass::StatusBarSubclass, dmlib_subclass::SubclassID::statusBar, ::CreateFontIndirectW(&lf));
	}

	/**
	 * @brief Removes the owner drawn subclass from a status bar control.
	 *
	 * Cleans up the `StatusBarData` and detaches the control's subclass proc.
	 *
	 * @param hWnd Handle to the status bar control.
	 *
	 * @see dmlib_subclass::StatusBarSubclass()
	 * @see DarkMode::setStatusBarCtrlSubclass()
	 */
	void removeStatusBarCtrlSubclass(HWND hWnd)
	{
		dmlib_subclass::RemoveSubclass<dmlib_subclass::StatusBarData>(hWnd, dmlib_subclass::StatusBarSubclass, dmlib_subclass::SubclassID::statusBar);
	}

	/**
	 * @brief Applies owner drawn subclassing to a status bar control.
	 *
	 * Overload wrapper that applies the subclass only if `p.m_subclass` is `true`.
	 *
	 * @param hWnd  Handle to the status bar control.
	 * @param p     Parameters controlling whether to apply subclassing.
	 *
	 * @see DarkMode::setStatusBarCtrlSubclass()
	 */
	static void setStatusBarCtrlSubclass(HWND hWnd, DarkModeParams p)
	{
		if (p.m_subclass)
		{
			DarkMode::setStatusBarCtrlSubclass(hWnd);
		}
	}

	/**
	 * @brief Applies owner drawn subclassing to a progress bar control.
	 *
	 * Retrieves the progress bar state information and passes it to the subclass data
	 * (`ProgressBarData`).
	 *
	 * @param hWnd Handle to the progress bar control.
	 *
	 * @note Uses `PBM_GETSTATE` to determine the current visual state.
	 *
	 * @see dmlib_subclass::ProgressBarSubclass()
	 * @see DarkMode::removeProgressBarCtrlSubclass()
	 */
	void setProgressBarCtrlSubclass(HWND hWnd)
	{
		dmlib_subclass::SetSubclass<dmlib_subclass::ProgressBarData>(hWnd, dmlib_subclass::ProgressBarSubclass, dmlib_subclass::SubclassID::progressBar, hWnd);
	}

	/**
	 * @brief Removes the owner drawn subclass from a progress bar control.
	 *
	 * Cleans up the `ProgressBarData` and detaches the control's subclass proc.
	 *
	 * @param hWnd Handle to the progress bar control.
	 *
	 * @see dmlib_subclass::ProgressBarSubclass()
	 * @see DarkMode::setProgressBarCtrlSubclass()
	 */
	void removeProgressBarCtrlSubclass(HWND hWnd)
	{
		dmlib_subclass::RemoveSubclass<dmlib_subclass::ProgressBarData>(hWnd, dmlib_subclass::ProgressBarSubclass, dmlib_subclass::SubclassID::progressBar);
	}

	/**
	 * @brief Applies theming or subclassing to a progress bar control based on style and parameters.
	 *
	 * Conditionally applies either the classic theme or applies the owner drawn subclassing
	 * depending on the control style and `DarkModeParams`.
	 *
	 * Behavior:
	 * - If `p.m_theme` is `true` and the control uses `PBS_MARQUEE`, applies classic theme.
	 * - Otherwise, if `p.m_subclass` is `true`, applies owner drawn subclassing.
	 *
	 * @param hWnd  Handle to the progress bar control.
	 * @param p     Parameters controlling whether to apply theming or subclassing.
	 *
	 * @see DarkMode::setProgressBarClassicTheme()
	 * @see DarkMode::setProgressBarCtrlSubclass()
	 */
	static void setProgressBarCtrlSubclass(HWND hWnd, DarkModeParams p)
	{
		const auto nStyle = ::GetWindowLongPtr(hWnd, GWL_STYLE);
		if (p.m_theme && (nStyle & PBS_MARQUEE) == PBS_MARQUEE)
		{
			DarkMode::setProgressBarClassicTheme(hWnd);
		}
		else if (p.m_subclass)
		{
			DarkMode::setProgressBarCtrlSubclass(hWnd);
		}
	}

	/**
	 * @brief Applies workaround subclassing to a static control to handle visual glitch in disabled state.
	 *
	 * Retrieves the static control enabled state information and passes it to the subclass data
	 * (`StaticTextData`) to handle visual glitch with static text in disabled state
	 * via handling `WM_ENABLE` message.
	 *
	 * @param hWnd Handle to the static control.
	 *
	 * @note
	 * - Uses `IsWindowEnabled` to determine the current enabled state.
	 * - Works only if `WM_ENABLE` message is sent.
	 *
	 * @see dmlib_subclass::StaticTextSubclass()
	 * @see DarkMode::removeStaticTextCtrlSubclass()
	 */
	void setStaticTextCtrlSubclass(HWND hWnd)
	{
		dmlib_subclass::SetSubclass<dmlib_subclass::StaticTextData>(hWnd, dmlib_subclass::StaticTextSubclass, dmlib_subclass::SubclassID::staticText, hWnd);
	}

	/**
	 * @brief Removes the workaround subclass from a static control.
	 *
	 * Cleans up the `StaticTextData` and detaches the control's subclass proc.
	 *
	 * @param hWnd Handle to the static control.
	 *
	 * @see dmlib_subclass::StaticTextSubclass()
	 * @see DarkMode::setStaticTextCtrlSubclass()
	 */
	void removeStaticTextCtrlSubclass(HWND hWnd)
	{
		dmlib_subclass::RemoveSubclass<dmlib_subclass::StaticTextData>(hWnd, dmlib_subclass::StaticTextSubclass, dmlib_subclass::SubclassID::staticText);
	}

	/**
	 * @brief Applies workaround subclassing to a static control.
	 *
	 * Overload wrapper that applies the subclass only if `p.m_subclass` is `true`.
	 *
	 * @param hWnd  Handle to the static control.
	 * @param p     Parameters controlling whether to apply subclassing.
	 *
	 * @see DarkMode::setStaticTextCtrlSubclass()
	 */
	static void setStaticTextCtrlSubclass(HWND hWnd, DarkModeParams p)
	{
		if (p.m_subclass)
		{
			DarkMode::setStaticTextCtrlSubclass(hWnd);
		}
	}

	/**
	 * @brief Applies owner drawn subclassing to a IP address control.
	 *
	 * Handles custom colors for itself and its child edit controls.
	 *
	 * @param[in] hWnd Handle to the IP address control.
	 *
	 * @see dmlib_subclass::IPAddressSubclass()
	 * @see DarkMode::removeIPAddressCtrlSubclass()
	 */
	void setIPAddressCtrlSubclass(HWND hWnd)
	{
		dmlib_subclass::SetSubclass(hWnd, dmlib_subclass::IPAddressSubclass, dmlib_subclass::SubclassID::ipAddress);
	}

	/**
	 * @brief Removes the owner drawn subclass from a IP address control.
	 *
	 * @param[in] hWnd Handle to the IP address control.
	 *
	 * @see dmlib_subclass::IPAddressSubclass()
	 * @see DarkMode::setIPAddressCtrlSubclass()
	 */
	void removeIPAddressCtrlSubclass(HWND hWnd)
	{
		dmlib_subclass::RemoveSubclass(hWnd, dmlib_subclass::IPAddressSubclass, dmlib_subclass::SubclassID::ipAddress);
	}

	/**
	 * @brief Applies owner drawn subclassing to a IP address control and adjusts its border style.
	 *
	 * Overload wrapper that applies the subclass only if `p.m_subclass` is `true`.
	 * Adjusts border style depending on used dark mode state.
	 *
	 * @param[in]   hWnd    Handle to the IP address control.
	 * @param[in]   p       Parameters controlling whether to apply subclassing.
	 *
	 * @see DarkMode::setIPAddressCtrlSubclass()
	 */
	static void setIPAddressCtrlSubclass(HWND hWnd, DarkModeParams p)
	{
		if (p.m_subclass)
		{
			DarkMode::setIPAddressCtrlSubclass(hWnd);
		}

		if (p.m_theme)
		{
			DarkMode::replaceClientEdgeWithBorderSafe(hWnd);
		}
	}

	/**
	 * @brief Applies theming to a tree view control.
	 *
	 * Sets custom text and background colors, applies a themed window style,
	 * and applies themed tooltips for tree view items.
	 *
	 * @param hWnd  Handle to the tree view control.
	 * @param p     Parameters controlling whether to apply theming.
	 *
	 * @see DarkMode::setTreeViewWindowTheme()
	 * @see DarkMode::setDarkTooltips()
	 */
	static void setTreeViewCtrlTheme(HWND hWnd, DarkModeParams p)
	{
		if (p.m_theme)
		{
			TreeView_SetTextColor(hWnd, DarkMode::getViewTextColor());
			TreeView_SetBkColor(hWnd, DarkMode::getViewBackgroundColor());

			DarkMode::setTreeViewWindowThemeEx(hWnd, p.m_theme);
			DarkMode::setDarkTooltips(hWnd, static_cast<int>(ToolTipsType::treeview));
		}
	}

	/**
	 * @brief Applies subclassing to a rebar control.
	 *
	 * Applies window subclassing to handle `WM_ERASEBKGND` message.
	 *
	 * @param hWnd  Handle to the rebar control.
	 * @param p     Parameters controlling whether to apply subclassing.
	 *
	 * @see DarkMode::setWindowEraseBgSubclass()
	 */
	static void setRebarCtrlSubclass(HWND hWnd, DarkModeParams p)
	{
		if (p.m_subclass)
		{
			DarkMode::setWindowEraseBgSubclass(hWnd);
		}
	}

	/**
	 * @brief Applies theming to a toolbar control.
	 *
	 * Sets custom colors for line above toolbar panel
	 * and applies themed tooltips for toolbar buttons.
	 *
	 * @param hWnd  Handle to the toolbar control.
	 * @param p     Parameters controlling whether to apply theming.
	 *
	 * @see DarkMode::setDarkLineAbovePanelToolbar()
	 * @see DarkMode::setDarkTooltips()
	 */
	static void setToolbarCtrlTheme(HWND hWnd, DarkModeParams p)
	{
		if (p.m_theme)
		{
			DarkMode::setDarkLineAbovePanelToolbar(hWnd);
			DarkMode::setDarkTooltips(hWnd, static_cast<int>(ToolTipsType::toolbar));
		}
	}

	/**
	 * @brief Applies theming to a scroll bar control.
	 *
	 * @param hWnd  Handle to the scroll bar control.
	 * @param p     Parameters controlling whether to apply theming.
	 *
	 * @see DarkMode::setDarkScrollBar()
	 */
	static void setScrollBarCtrlTheme(HWND hWnd, DarkModeParams p)
	{
		if (p.m_theme)
		{
			DarkMode::setDarkScrollBar(hWnd);
		}
	}

	/**
	 * @brief Applies theming to a SysLink control.
	 *
	 * Overload that enable `WM_CTLCOLORSTATIC` message handling
	 * depending on `DarkModeParams` for the syslink control.
	 *
	 * @param hWnd  Handle to the SysLink control.
	 * @param p     Parameters controlling whether to apply theming.
	 *
	 * @see DarkMode::enableSysLinkCtrlCtlColor()
	 */
	static void enableSysLinkCtrlCtlColor(HWND hWnd, DarkModeParams p)
	{
		if (p.m_theme)
		{
			DarkMode::enableSysLinkCtrlCtlColor(hWnd);
		}
	}

	/**
	 * @brief Applies theming to a trackbar control.
	 *
	 * Sets transparent background via `TBS_TRANSPARENTBKGND` flag
	 * and applies themed tooltips for trackbar buttons.
	 *
	 * @param hWnd  Handle to the trackbar control.
	 * @param p     Parameters controlling whether to apply theming.
	 *
	 * @see DarkMode::setWindowStyle()
	 * @see DarkMode::setDarkTooltips()
	 */
	static void setTrackbarCtrlTheme(HWND hWnd, DarkModeParams p)
	{
		if (p.m_theme)
		{
			DarkMode::setWindowStyle(hWnd, DarkMode::isEnabled(), TBS_TRANSPARENTBKGND);
			DarkMode::setDarkTooltips(hWnd, static_cast<int>(ToolTipsType::trackbar));
		}
	}

	/**
	 * @brief Applies theming to a rich edit control.
	 *
	 * @param hWnd  Handle to the rich edit control.
	 * @param p     Parameters controlling whether to apply theming.
	 *
	 * @see DarkMode::setDarkRichEdit()
	 */
	static void setRichEditCtrlTheme(HWND hWnd, DarkModeParams p)
	{
		if (p.m_theme)
		{
			DarkMode::setDarkRichEdit(hWnd);
		}
	}

	/**
	 * @brief Callback function used to enumerate and apply theming/subclassing to child controls.
	 *
	 * Called in `setChildCtrlsSubclassAndTheme()` and `setChildCtrlsTheme()`
	 * to inspect each child window's class name and apply appropriate theming
	 * and/or subclassing logic based on control type.
	 *
	 * @param[in]   hWnd    Handle to the window being enumerated.
	 * @param[in]   lParam  Pointer to a `DarkModeParams` structure containing theming flags and settings.
	 * @return `TRUE`   to continue enumeration.
	 *
	 * @note
	 * - Currently handles these controls:
	 *      `WC_BUTTON`, `WC_STATIC`, `WC_COMBOBOX`, `WC_EDIT`, `WC_LISTBOX`,
	 *      `WC_LISTVIEW`, `WC_TREEVIEW`, `REBARCLASSNAME`, `TOOLBARCLASSNAME`,
	 *      `UPDOWN_CLASS`, `WC_TABCONTROL`, `STATUSCLASSNAME`, `WC_SCROLLBAR`,
	 *      `WC_COMBOBOXEX`, `PROGRESS_CLASS`, `WC_LINK`, `TRACKBAR_CLASS`,
	 *      `RICHEDIT_CLASS`, `MSFTEDIT_CLASS`, and `WC_IPADDRESS`
	 * - The `#32770` dialog class is commented out for debugging purposes.
	 *
	 * @see DarkMode::setChildCtrlsSubclassAndTheme()
	 * @see DarkMode::setChildCtrlsTheme()
	 * @see DarkModeParams
	 * @see DarkMode::setBtnCtrlSubclassAndTheme()
	 * @see DarkMode::setStaticTextCtrlSubclass()
	 * @see DarkMode::setComboBoxCtrlSubclassAndTheme()
	 * @see DarkMode::setCustomBorderForListBoxOrEditCtrlSubclassAndTheme()
	 * @see DarkMode::setListViewCtrlSubclassAndTheme()
	 * @see DarkMode::setTreeViewCtrlTheme()
	 * @see DarkMode::setRebarCtrlSubclass()
	 * @see DarkMode::setToolbarCtrlTheme()
	 * @see DarkMode::setUpDownCtrlSubclassAndTheme()
	 * @see DarkMode::setTabCtrlSubclassAndTheme()
	 * @see DarkMode::setStatusBarCtrlSubclass()
	 * @see DarkMode::setScrollBarCtrlTheme()
	 * @see DarkMode::setComboBoxExCtrlSubclass()
	 * @see DarkMode::setProgressBarCtrlSubclass()
	 * @see DarkMode::enableSysLinkCtrlCtlColor()
	 * @see DarkMode::setTrackbarCtrlTheme()
	 * @see DarkMode::setRichEditCtrlTheme()
	 * @see DarkMode::setIPAddressCtrlSubclass()
	 */
	static BOOL CALLBACK DarkEnumChildProc(HWND hWnd, LPARAM lParam)
	{
		const auto& p = *reinterpret_cast<DarkModeParams*>(lParam);
		const std::wstring className = dmlib_subclass::getWndClassName(hWnd);

		if (className == WC_BUTTON)
		{
			DarkMode::setBtnCtrlSubclassAndTheme(hWnd, p);
			return TRUE;
		}

		if (className == WC_STATIC)
		{
			DarkMode::setStaticTextCtrlSubclass(hWnd, p);
			return TRUE;
		}

		if (className == WC_COMBOBOX)
		{
			DarkMode::setComboBoxCtrlSubclassAndTheme(hWnd, p);
			return TRUE;
		}

		if (className == WC_EDIT)
		{
			DarkMode::setCustomBorderForListBoxOrEditCtrlSubclassAndTheme(hWnd, p, false);
			return TRUE;
		}

		if (className == WC_LISTBOX)
		{
			DarkMode::setCustomBorderForListBoxOrEditCtrlSubclassAndTheme(hWnd, p, true);
			return TRUE;
		}

		if (className == WC_LISTVIEW)
		{
			DarkMode::setListViewCtrlSubclassAndTheme(hWnd, p);
			return TRUE;
		}

		if (className == WC_TREEVIEW)
		{
			DarkMode::setTreeViewCtrlTheme(hWnd, p);
			return TRUE;
		}

		if (className == REBARCLASSNAME)
		{
			DarkMode::setRebarCtrlSubclass(hWnd, p);
			return TRUE;
		}

		if (className == TOOLBARCLASSNAME)
		{
			DarkMode::setToolbarCtrlTheme(hWnd, p);
			return TRUE;
		}

		if (className == UPDOWN_CLASS)
		{
			DarkMode::setUpDownCtrlSubclassAndTheme(hWnd, p);
			return TRUE;
		}

		if (className == WC_TABCONTROL)
		{
			DarkMode::setTabCtrlSubclassAndTheme(hWnd, p);
			return TRUE;
		}

		if (className == STATUSCLASSNAME)
		{
			DarkMode::setStatusBarCtrlSubclass(hWnd, p);
			return TRUE;
		}

		if (className == WC_SCROLLBAR)
		{
			DarkMode::setScrollBarCtrlTheme(hWnd, p);
			return TRUE;
		}

		if (className == WC_COMBOBOXEX)
		{
			DarkMode::setComboBoxExCtrlSubclass(hWnd, p);
			return TRUE;
		}

		if (className == PROGRESS_CLASS)
		{
			DarkMode::setProgressBarCtrlSubclass(hWnd, p);
			return TRUE;
		}

		if (className == WC_LINK)
		{
			DarkMode::enableSysLinkCtrlCtlColor(hWnd, p);
			return TRUE;
		}

		if (className == TRACKBAR_CLASS)
		{
			DarkMode::setTrackbarCtrlTheme(hWnd, p);
			return TRUE;
		}

		if (className == RICHEDIT_CLASS || className == MSFTEDIT_CLASS) // rich edit controls 2.0, 3.0, and 4.1
		{
			DarkMode::setRichEditCtrlTheme(hWnd, p);
			return TRUE;
		}

		if (className == WC_IPADDRESS)
		{
			DarkMode::setIPAddressCtrlSubclass(hWnd, p);
			return TRUE;
		}
#if 0 // for debugging
		if (className == L"#32770") // dialog
		{
			return TRUE;
		}
#endif
		return TRUE;
	}

	/**
	 * @brief Applies theming and/or subclassing to all child controls of a parent window.
	 *
	 * Enumerates all child windows of the specified parent and dispatches them to
	 * `DarkEnumChildProc`, which applies control-specific theming and/or subclassing logic
	 * based on their class name and the provided parameters.
	 *
	 * Mainly used when initializing parent control.
	 *
	 * @param hParent   Handle to the parent window whose child controls will be themed and/or subclassed.
	 * @param subclass  Whether to apply subclassing.
	 * @param theme     Whether to apply theming.
	 *
	 * @see DarkMode::setChildCtrlsSubclassAndTheme()
	 * @see DarkMode::DarkEnumChildProc()
	 * @see DarkModeParams
	 */
	void setChildCtrlsSubclassAndThemeEx(HWND hParent, bool subclass, bool theme)
	{
		DarkModeParams p{
			DarkMode::isExperimentalActive() ? L"DarkMode_Explorer" : nullptr
			, subclass
			, theme
		};

		::EnumChildWindows(hParent, DarkMode::DarkEnumChildProc, reinterpret_cast<LPARAM>(&p));
	}

	/**
	 * @brief Wrapper for `DarkMode::setChildCtrlsSubclassAndThemeEx`.
	 *
	 * Forwards to `DarkMode::setChildCtrlsSubclassAndThemeEx` with `subclass` and `theme` parameters set as `true`.
	 *
	 * @param hParent Handle to the parent window whose child controls will be themed and/or subclassed.
	 *
	 * @see DarkMode::setChildCtrlsSubclassAndThemeEx()
	 */
	void setChildCtrlsSubclassAndTheme(HWND hParent)
	{
		DarkMode::setChildCtrlsSubclassAndThemeEx(hParent, true, true);
	}

	/**
	 * @brief Applies theming to all child controls of a parent window.
	 *
	 * Enumerates child windows of the specified parent and applies theming without subclassing.
	 * The theming behavior adapts based on OS support and compile-time flags.
	 * If `_DARKMODELIB_ALLOW_OLD_OS > 1` is true, theming is applied unconditionally.
	 * Otherwise, theming is applied only if the OS is Windows 10 or newer.
	 * The function delegates to `setChildCtrlsSubclassAndTheme()` with appropriate flags.
	 *
	 * Mainly used when changing mode.
	 *
	 * @param hParent Handle to the parent window whose child controls will be themed.
	 *
	 * @see DarkMode::setChildCtrlsSubclassAndTheme()
	 */
	void setChildCtrlsTheme(HWND hParent)
	{
#if defined(_DARKMODELIB_ALLOW_OLD_OS) && (_DARKMODELIB_ALLOW_OLD_OS > 1)
		DarkMode::setChildCtrlsSubclassAndThemeEx(hParent, false, true);
#else
		DarkMode::setChildCtrlsSubclassAndThemeEx(hParent, false, DarkMode::isAtLeastWindows10());
#endif
	}

	/**
	 * @brief Applies window subclassing to handle `WM_ERASEBKGND` message.
	 *
	 * @param hWnd Handle to the control to subclass.
	 *
	 * @see dmlib_subclass::WindowEraseBgSubclass()
	 * @see DarkMode::removeWindowEraseBgSubclass()
	 */
	void setWindowEraseBgSubclass(HWND hWnd)
	{
		dmlib_subclass::SetSubclass(hWnd, dmlib_subclass::WindowEraseBgSubclass, dmlib_subclass::SubclassID::windowEraseBg);
	}

	/**
	 * @brief Removes the subclass used for `WM_ERASEBKGND` message handling.
	 *
	 * Detaches the window's subclass proc used for `WM_ERASEBKGND` message handling.
	 *
	 * @param hWnd Handle to the previously subclassed window.
	 *
	 * @see dmlib_subclass::WindowEraseBgSubclass()
	 * @see DarkMode::removeWindowEraseBgSubclass()
	 */
	void removeWindowEraseBgSubclass(HWND hWnd)
	{
		dmlib_subclass::RemoveSubclass(hWnd, dmlib_subclass::WindowEraseBgSubclass, dmlib_subclass::SubclassID::windowEraseBg);
	}

	/**
	 * @brief Applies window subclassing to handle `WM_CTLCOLOR*` messages.
	 *
	 * Enable custom colors for edit, listbox, static, and dialog elements
	 * via @ref dmlib_subclass::WindowCtlColorSubclass.
	 *
	 * @param hWnd Handle to the parent or composite control (dialog, rebar, toolbar, ...) to subclass.
	 *
	 * @see dmlib_subclass::WindowCtlColorSubclass()
	 * @see DarkMode::removeWindowCtlColorSubclass()
	 */
	void setWindowCtlColorSubclass(HWND hWnd)
	{
		dmlib_subclass::SetSubclass(hWnd, dmlib_subclass::WindowCtlColorSubclass, dmlib_subclass::SubclassID::windowCtlColor);
	}

	/**
	 * @brief Removes the subclass used for `WM_CTLCOLOR*` messages handling.
	 *
	 * Detaches the window's subclass proc used for `WM_CTLCOLOR*` messages handling.
	 *
	 * @param hWnd Handle to the previously subclassed window.
	 *
	 * @see dmlib_subclass::WindowCtlColorSubclass()
	 * @see DarkMode::setWindowCtlColorSubclass()
	 */
	void removeWindowCtlColorSubclass(HWND hWnd)
	{
		dmlib_subclass::RemoveSubclass(hWnd, dmlib_subclass::WindowCtlColorSubclass, dmlib_subclass::SubclassID::windowCtlColor);
	}

	/**
	 * @brief Applies window subclassing for handling `NM_CUSTOMDRAW` notifications for custom drawing.
	 *
	 * Installs @ref dmlib_subclass::WindowNotifySubclass.
	 * Enables handling of `WM_NOTIFY` `NM_CUSTOMDRAW` notifications for custom drawing
	 * behavior for supported controls.
	 *
	 * @param hWnd Handle to the window with child which support `NM_CUSTOMDRAW`.
	 *
	 * @see dmlib_subclass::WindowNotifySubclass()
	 * @see DarkMode::removeWindowNotifyCustomDrawSubclass()
	 */
	void setWindowNotifyCustomDrawSubclass(HWND hWnd)
	{
		dmlib_subclass::SetSubclass(hWnd, dmlib_subclass::WindowNotifySubclass, dmlib_subclass::SubclassID::windowNotify);
	}

	/**
	 * @brief Removes the subclass used for handling `NM_CUSTOMDRAW` notifications for custom drawing.
	 *
	 * Detaches the window's subclass proc used for handling `NM_CUSTOMDRAW` notifications for custom drawing.
	 *
	 * @param hWnd Handle to the previously subclassed window.
	 *
	 * @see dmlib_subclass::WindowNotifySubclass()
	 * @see DarkMode::setWindowNotifyCustomDrawSubclass()
	 */
	void removeWindowNotifyCustomDrawSubclass(HWND hWnd)
	{
		dmlib_subclass::RemoveSubclass(hWnd, dmlib_subclass::WindowNotifySubclass, dmlib_subclass::SubclassID::windowNotify);
	}

	/**
	 * @brief Applies window subclassing for menu bar themed custom drawing.
	 *
	 * Installs @ref DarkMode::WindowMenuBarSubclass with an associated `ThemeData` instance
	 * for the `VSCLASS_MENU` visual style. Enables custom drawing
	 * behavior for menu bar.
	 *
	 * @param hWnd Handle to the window with a menu bar.
	 *
	 * @see dmlib_subclass::WindowMenuBarSubclass()
	 * @see DarkMode::removeWindowMenuBarSubclass()
	 */
	void setWindowMenuBarSubclass(HWND hWnd)
	{
		dmlib_subclass::SetSubclass<dmlib_subclass::ThemeData>(hWnd, dmlib_subclass::WindowMenuBarSubclass, dmlib_subclass::SubclassID::windowMenuBar, VSCLASS_MENU);
	}

	/**
	 * @brief Removes the subclass used for menu bar themed custom drawing.
	 *
	 * Detaches the window's subclass proc used for menu bar themed custom drawing.
	 *
	 * @param hWnd Handle to the previously subclassed window.
	 *
	 * @see dmlib_subclass::WindowMenuBarSubclass()
	 * @see DarkMode::setWindowMenuBarSubclass()
	 */
	void removeWindowMenuBarSubclass(HWND hWnd)
	{
		dmlib_subclass::RemoveSubclass<dmlib_subclass::ThemeData>(hWnd, dmlib_subclass::WindowMenuBarSubclass, dmlib_subclass::SubclassID::windowMenuBar);
	}

	/**
	 * @brief Applies window subclassing to handle `WM_SETTINGCHANGE` message.
	 *
	 * Enable monitoring WM_SETTINGCHANGE message,
	 * allowing the app to respond to system-wide dark mode change.
	 *
	 * @param hWnd Handle to the main window.
	 *
	 * @see DarkMode::WindowSettingChangeSubclass()
	 * @see DarkMode::removeWindowSettingChangeSubclass()
	 */
	void setWindowSettingChangeSubclass(HWND hWnd)
	{
		dmlib_subclass::SetSubclass(hWnd, dmlib_subclass::WindowSettingChangeSubclass, dmlib_subclass::SubclassID::windowSettingChange);
	}

	/**
	 * @brief Removes the subclass used for `WM_SETTINGCHANGE` message handling.
	 *
	 * Detaches the window's subclass proc used for `WM_SETTINGCHANGE` messages handling.
	 *
	 * @param hWnd Handle to the previously subclassed window.
	 *
	 * @see DarkMode::WindowSettingChangeSubclass()
	 * @see DarkMode::setWindowSettingChangeSubclass()
	 */
	void removeWindowSettingChangeSubclass(HWND hWnd)
	{
		dmlib_subclass::RemoveSubclass(hWnd, dmlib_subclass::WindowSettingChangeSubclass, dmlib_subclass::SubclassID::windowSettingChange);
	}

	/**
	 * @brief Configures the SysLink control to be affected by `WM_CTLCOLORSTATIC` message.
	 *
	 * Configures all items to either use default system link colors if in classic mode,
	 * or to be affected by `WM_CTLCOLORSTATIC` message from its parent.
	 *
	 * @param hWnd Handle to the SysLink control.
	 *
	 * @note Will affect all items, even if it's static (non-clickable).
	 */
	void enableSysLinkCtrlCtlColor(HWND hWnd)
	{
		LITEM lItem{};
		lItem.iLink = 0;
		lItem.mask = LIF_ITEMINDEX | LIF_STATE;
		lItem.state = DarkMode::isEnabled() ? LIS_DEFAULTCOLORS : 0;
		lItem.stateMask = LIS_DEFAULTCOLORS;
		while (::SendMessage(hWnd, LM_SETITEM, 0, reinterpret_cast<LPARAM>(&lItem)) == TRUE)
		{
			++lItem.iLink;
		}
	}

	/**
	 * @brief Sets dark title bar and optional Windows 11 features.
	 *
	 * For Windows 10 (2004+) and newer, this function configures the dark title bar using
	 * `DWMWA_USE_IMMERSIVE_DARK_MODE`. On Windows 11, if `useWin11Features` is `true`, it
	 * additionally applies:
	 * - Rounded corners (`DWMWA_WINDOW_CORNER_PREFERENCE`)
	 * - Border color (`DWMWA_BORDER_COLOR`)
	 * - Mica backdrop (`DWMWA_SYSTEMBACKDROP_TYPE`) if allowed and compatible
	 * - Static text color for text and dialog background color for background
	 *   (`DWMWA_CAPTION_COLOR`, `DWMWA_TEXT_COLOR`),
	 *   only when frames are not extended to full window
	 *
	 * If `_DARKMODELIB_ALLOW_OLD_OS` is defined with non-zero unsigned value
	 * and running on pre-2004 builds, fallback behavior will enable dark title bars via undocumented APIs.
	 *
	 * @param hWnd              Handle to the top-level window.
	 * @param useWin11Features  `true` to enable Windows 11 specific features such as Mica and rounded corners.
	 *
	 * @note Requires Windows 10 version 2004 (build 19041) or later.
	 *
	 * @see DwmSetWindowAttribute
	 * @see DwmExtendFrameIntoClientArea
	 */
	void setDarkTitleBarEx(HWND hWnd, bool useWin11Features)
	{
		static constexpr DWORD win10Build2004 = 19041;
		static constexpr DWORD win11Mica = 22621;
		if (DarkMode::getWindowsBuildNumber() >= win10Build2004)
		{
			const BOOL useDark = DarkMode::isExperimentalActive() ? TRUE : FALSE;
			::DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDark, sizeof(useDark));

			if (useWin11Features && DarkMode::isAtLeastWindows11())
			{
				::DwmSetWindowAttribute(hWnd, DWMWA_WINDOW_CORNER_PREFERENCE, &g_dmCfg.m_roundCorner, sizeof(g_dmCfg.m_roundCorner));
				::DwmSetWindowAttribute(hWnd, DWMWA_BORDER_COLOR, &g_dmCfg.m_borderColor, sizeof(g_dmCfg.m_borderColor));

				bool canColorizeTitleBar = true;

				if (DarkMode::getWindowsBuildNumber() >= win11Mica)
				{
					if (g_dmCfg.m_micaExtend && g_dmCfg.m_mica != DWMSBT_AUTO && !DarkMode::isWindowsModeEnabled() && (g_dmCfg.m_dmType == DarkModeType::dark))
					{
						static constexpr MARGINS margins{ -1, 0, 0, 0 };
						::DwmExtendFrameIntoClientArea(hWnd, &margins);
					}

					::DwmSetWindowAttribute(hWnd, DWMWA_SYSTEMBACKDROP_TYPE, &g_dmCfg.m_mica, sizeof(g_dmCfg.m_mica));

					canColorizeTitleBar = !g_dmCfg.m_micaExtend;
				}

				canColorizeTitleBar = g_dmCfg.m_colorizeTitleBar && canColorizeTitleBar && DarkMode::isEnabled();
				const COLORREF clrDlg = canColorizeTitleBar ? DarkMode::getDlgBackgroundColor() : DWMWA_COLOR_DEFAULT;
				const COLORREF clrText = canColorizeTitleBar ? DarkMode::getTextColor() : DWMWA_COLOR_DEFAULT;
				::DwmSetWindowAttribute(hWnd, DWMWA_CAPTION_COLOR, &clrDlg, sizeof(clrDlg));
				::DwmSetWindowAttribute(hWnd, DWMWA_TEXT_COLOR, &clrText, sizeof(clrText));
			}
		}
#if defined(_DARKMODELIB_ALLOW_OLD_OS) && (_DARKMODELIB_ALLOW_OLD_OS > 0)
		else
		{
			dmlib_win32api::AllowDarkModeForWindow(hWnd, DarkMode::isExperimentalActive());
			dmlib_win32api:::RefreshTitleBarThemeColor(hWnd);
		}
#endif
		// on Windows 10 title bar needs refresh when changing colors
		if (DarkMode::isAtLeastWindows10() && !DarkMode::isAtLeastWindows11())
		{
			const bool isActive = (hWnd == ::GetActiveWindow()) && (hWnd == ::GetForegroundWindow());
			::SendMessage(hWnd, WM_NCACTIVATE, static_cast<WPARAM>(!isActive), 0);
			::SendMessage(hWnd, WM_NCACTIVATE, static_cast<WPARAM>(isActive), 0);
		}
	}

	/**
	 * @brief Sets dark mode title bar on supported Windows versions.
	 *
	 * Delegates to @ref setDarkTitleBarEx with `useWin11Features = false`.
	 *
	 * @param hWnd Handle to the top-level window.
	 *
	 * @see DarkMode::setDarkTitleBarEx()
	 */
	void setDarkTitleBar(HWND hWnd)
	{
		DarkMode::setDarkTitleBarEx(hWnd, false);
	}

	/**
	 * @brief Applies an experimental visual style to the specified window, if supported.
	 *
	 * When experimental features are supported and active,
	 * this function enables dark experimental visual style on the window.
	 *
	 * @param hWnd              Handle to the target window or control.
	 * @param themeClassName    Name of the theme class to apply (e.g. L"Explorer", "ItemsView").
	 *
	 * @note This function is a no-op if experimental theming is not supported on the current OS.
	 *
	 * @see DarkMode::isExperimentalSupported()
	 * @see DarkMode::isExperimentalActive()
	 * @see dmlib_win32api::AllowDarkModeForWindow()
	 * @see DarkMode::setDarkThemeExperimental()
	 */
	void setDarkThemeExperimentalEx(HWND hWnd, const wchar_t* themeClassName)
	{
		if (DarkMode::isExperimentalSupported())
		{
			dmlib_win32api::AllowDarkModeForWindow(hWnd, DarkMode::isExperimentalActive());
			::SetWindowTheme(hWnd, themeClassName, nullptr);
		}
	}

	/**
	 * @brief Applies an experimental Explorer visual style to the specified window, if supported.
	 *
	 * Forwards to `DarkMode::setDarkThemeExperimentalEx` with `themeClassName` as `L"Explorer"`.
	 *
	 * @param hWnd Handle to the target window or control.
	 *
	 * @see DarkMode::setDarkThemeExperimentalEx()
	 */
	void setDarkThemeExperimental(HWND hWnd)
	{
		DarkMode::setDarkThemeExperimentalEx(hWnd, L"Explorer");
	}

	/**
	 * @brief Applies "DarkMode_Explorer" visual style if experimental mode is active.
	 *
	 * Useful for controls like list views or tree views to use dark scroll bars
	 * and explorer style theme in supported environments.
	 *
	 * @param hWnd Handle to the control or window to theme.
	 */
	void setDarkExplorerTheme(HWND hWnd)
	{
		::SetWindowTheme(hWnd, DarkMode::isExperimentalActive() ? L"DarkMode_Explorer" : nullptr, nullptr);
	}

	/**
	 * @brief Applies "DarkMode_Explorer" visual style to scroll bars.
	 *
	 * Convenience wrapper that calls @ref DarkMode::setDarkExplorerTheme to apply dark scroll bar
	 * for compatible controls (e.g. list views, tree views).
	 *
	 * @param hWnd Handle to the control with scroll bars.
	 *
	 * @see DarkMode::setDarkExplorerTheme()
	 */
	void setDarkScrollBar(HWND hWnd)
	{
		DarkMode::setDarkExplorerTheme(hWnd);
	}

	/**
	 * @brief Applies "DarkMode_Explorer" visual style to tooltip controls based on context.
	 *
	 * Selects the appropriate `GETTOOLTIPS` message depending on the control type
	 * (e.g. toolbar, list view, tree view, tab bar) to retrieve the tooltip handle.
	 * If `ToolTipsType::tooltip` is specified, applies theming directly to `hWnd`.
	 *
	 * Internally calls @ref DarkMode::setDarkExplorerTheme to set dark tooltip.
	 *
	 * @param hWnd          Handle to the parent control or tooltip.
	 * @param tooltipType   The tooltip context type (toolbar, list view, etc.).
	 *
	 * @see DarkMode::setDarkExplorerTheme()
	 * @see ToolTipsType
	 */
	void setDarkTooltips(HWND hWnd, int tooltipType)
	{
		const auto type = static_cast<ToolTipsType>(tooltipType);
		UINT msg = 0;
		switch (type)
		{
			case DarkMode::ToolTipsType::toolbar:
			{
				msg = TB_GETTOOLTIPS;
				break;
			}

			case DarkMode::ToolTipsType::listview:
			{
				msg = LVM_GETTOOLTIPS;
				break;
			}

			case DarkMode::ToolTipsType::treeview:
			{
				msg = TVM_GETTOOLTIPS;
				break;
			}

			case DarkMode::ToolTipsType::tabbar:
			{
				msg = TCM_GETTOOLTIPS;
				break;
			}

			case DarkMode::ToolTipsType::trackbar:
			{
				msg = TBM_GETTOOLTIPS;
				break;
			}

			case DarkMode::ToolTipsType::rebar:
			{
				msg = RB_GETTOOLTIPS;
				break;
			}

			case DarkMode::ToolTipsType::tooltip:
			{
				msg = 0;
				break;
			}
		}

		if (msg == 0)
		{
			DarkMode::setDarkExplorerTheme(hWnd);
		}
		else
		{
			auto hTips = reinterpret_cast<HWND>(::SendMessage(hWnd, msg, 0, 0));
			if (hTips != nullptr)
			{
				DarkMode::setDarkExplorerTheme(hTips);
			}
		}
	}

	/**
	 * @brief Sets the color of line above a toolbar control for non-classic mode.
	 *
	 * Sends `TB_SETCOLORSCHEME` to customize the line drawn above the toolbar.
	 * When non-classic mode is enabled, sets both `clrBtnHighlight` and `clrBtnShadow`
	 * to the dialog background color, otherwise uses system defaults.
	 *
	 * @param hWnd Handle to the toolbar control.
	 */
	void setDarkLineAbovePanelToolbar(HWND hWnd)
	{
		COLORSCHEME scheme{};
		scheme.dwSize = sizeof(COLORSCHEME);

		if (DarkMode::isEnabled())
		{
			scheme.clrBtnHighlight = DarkMode::getDlgBackgroundColor();
			scheme.clrBtnShadow = DarkMode::getDlgBackgroundColor();
		}
		else
		{
			scheme.clrBtnHighlight = CLR_DEFAULT;
			scheme.clrBtnShadow = CLR_DEFAULT;
		}

		::SendMessage(hWnd, TB_SETCOLORSCHEME, 0, reinterpret_cast<LPARAM>(&scheme));
	}

	/**
	 * @brief Applies an experimental Explorer visual style to a list view.
	 *
	 * Uses @ref DarkMode::setDarkThemeExperimental with the `"Explorer"` theme class to adapt
	 * list view visuals (e.g. scroll bars, selection color) for dark mode, if supported.
	 *
	 * @param hWnd Handle to the list view control.
	 *
	 * @see DarkMode::setDarkThemeExperimental()
	 */
	void setDarkListView(HWND hWnd)
	{
		DarkMode::setDarkThemeExperimental(hWnd);
	}

	/**
	 * @brief Replaces default list view checkboxes with themed dark-mode versions on Windows 11.
	 *
	 * If the list view uses `LVS_EX_CHECKBOXES` and is running on Windows 11 or later,
	 * this function manually draws the unchecked and checked checkbox visuals using
	 * themed drawing APIs, then inserts the resulting icons into the state image list.
	 *
	 * Uses `"DarkMode_Explorer::Button"` as the theme class if experimental dark mode is active;
	 * otherwise falls back to `VSCLASS_BUTTON`.
	 *
	 * @param hWnd Handle to the list view control with extended checkbox style.
	 *
	 * @note Does nothing on pre-Windows 11 systems or if checkboxes are not enabled.
	 */
	void setDarkListViewCheckboxes(HWND hWnd)
	{
		if (!DarkMode::isAtLeastWindows11())
		{
			return;
		}

		const auto lvExStyle = ListView_GetExtendedListViewStyle(hWnd);
		if ((lvExStyle & LVS_EX_CHECKBOXES) != LVS_EX_CHECKBOXES)
		{
			return;
		}

		HDC hdc = ::GetDC(nullptr);

		const bool useDark = DarkMode::isExperimentalActive() && DarkMode::isThemeDark();
		HTHEME hTheme = ::OpenThemeData(nullptr, useDark ? L"DarkMode_Explorer::Button" : VSCLASS_BUTTON);

		SIZE szBox{};
		::GetThemePartSize(hTheme, hdc, BP_CHECKBOX, CBS_UNCHECKEDNORMAL, nullptr, TS_DRAW, &szBox);

		const RECT rcBox{ 0, 0, szBox.cx, szBox.cy };

		auto hImgList = ListView_GetImageList(hWnd, LVSIL_STATE);
		if (hImgList == nullptr)
		{
			::CloseThemeData(hTheme);
			::ReleaseDC(nullptr, hdc);
			return;
		}
		::ImageList_RemoveAll(hImgList);

		HDC hBoxDC = ::CreateCompatibleDC(hdc);
		HBITMAP hBoxBmp = ::CreateCompatibleBitmap(hdc, szBox.cx, szBox.cy);
		HBITMAP hMaskBmp = ::CreateCompatibleBitmap(hdc, szBox.cx, szBox.cy);

		auto holdBmp = static_cast<HBITMAP>(::SelectObject(hBoxDC, hBoxBmp));
		::DrawThemeBackground(hTheme, hBoxDC, BP_CHECKBOX, CBS_UNCHECKEDNORMAL, &rcBox, nullptr);

		ICONINFO ii{};
		ii.fIcon = TRUE;
		ii.hbmColor = hBoxBmp;
		ii.hbmMask = hMaskBmp;

		HICON hIcon = ::CreateIconIndirect(&ii);
		if (hIcon != nullptr)
		{
			::ImageList_AddIcon(hImgList, hIcon);
			::DestroyIcon(hIcon);
			hIcon = nullptr;
		}

		::DrawThemeBackground(hTheme, hBoxDC, BP_CHECKBOX, CBS_CHECKEDNORMAL, &rcBox, nullptr);
		ii.hbmColor = hBoxBmp;

		hIcon = ::CreateIconIndirect(&ii);
		if (hIcon != nullptr)
		{
			::ImageList_AddIcon(hImgList, hIcon);
			::DestroyIcon(hIcon);
			hIcon = nullptr;
		}

		::SelectObject(hBoxDC, holdBmp);
		::DeleteObject(hMaskBmp);
		::DeleteObject(hBoxBmp);
		::DeleteDC(hBoxDC);
		::CloseThemeData(hTheme);
		::ReleaseDC(nullptr, hdc);
	}

	/**
	 * @brief Sets colors and edges for a RichEdit control.
	 *
	 * Determines if the control has `WS_BORDER` or `WS_EX_STATICEDGE`, and sets the background
	 * accordingly: uses control background color when edged, otherwise dialog background.
	 *
	 * In dark mode:
	 * - Sets background color via `EM_SETBKGNDCOLOR`
	 * - Updates default text color via `EM_SETCHARFORMAT`
	 * - Applies themed scroll bars using `DarkMode_Explorer::ScrollBar`
	 *
	 * When not in dark mode, restores default visual styles and coloring.
	 * Also conditionally swaps `WS_BORDER` and `WS_EX_STATICEDGE`.
	 *
	 * @param hWnd Handle to the RichEdit control.
	 *
	 * @see DarkMode::setWindowStyle()
	 * @see DarkMode::setWindowExStyle()
	 */
	void setDarkRichEdit(HWND hWnd)
	{
		const auto nStyle = ::GetWindowLongPtr(hWnd, GWL_STYLE);
		const bool hasBorder = (nStyle & WS_BORDER) == WS_BORDER;

		const auto nExStyle = ::GetWindowLongPtr(hWnd, GWL_EXSTYLE);
		const bool hasStaticEdge = (nExStyle & WS_EX_STATICEDGE) == WS_EX_STATICEDGE;

		if (DarkMode::isEnabled())
		{
			const COLORREF clrBg = (hasStaticEdge || hasBorder ? DarkMode::getCtrlBackgroundColor() : DarkMode::getDlgBackgroundColor());
			::SendMessage(hWnd, EM_SETBKGNDCOLOR, 0, static_cast<LPARAM>(clrBg));

			CHARFORMATW cf{};
			cf.cbSize = sizeof(CHARFORMATW);
			cf.dwMask = CFM_COLOR;
			cf.crTextColor = DarkMode::getTextColor();
			::SendMessage(hWnd, EM_SETCHARFORMAT, SCF_DEFAULT, reinterpret_cast<LPARAM>(&cf));

			::SetWindowTheme(hWnd, nullptr, L"DarkMode_Explorer::ScrollBar");
		}
		else
		{
			::SendMessage(hWnd, EM_SETBKGNDCOLOR, TRUE, 0);
			::SendMessage(hWnd, EM_SETCHARFORMAT, 0, 0);

			::SetWindowTheme(hWnd, nullptr, nullptr);
		}

		DarkMode::setWindowStyle(hWnd, DarkMode::isEnabled() && hasStaticEdge, WS_BORDER);
		DarkMode::setWindowExStyle(hWnd, !DarkMode::isEnabled() && hasBorder, WS_EX_STATICEDGE);
	}

	/**
	 * @brief Applies visual styles; ctl color message and child controls subclassings to a window safely.
	 *
	 * Ensures the specified window is not `nullptr` and then:
	 * - Enables the dark title bar
	 * - Subclasses the window for control ctl coloring
	 * - Applies theming and subclassing to child controls
	 *
	 *
	 * @param hWnd              Handle to the window. No action taken if `nullptr`.
	 * @param useWin11Features  `true` to enable Windows 11 specific styling like Mica or rounded corners.
	 *
	 * @note Should not be used in combination with @ref DarkMode::setDarkWndNotifySafeEx
	 *       and @ref DarkMode::setDarkWndNotifySafe to avoid overlapping styling logic.
	 *
	 * @see DarkMode::setDarkWndNotifySafeEx()
	 * @see DarkMode::setDarkWndNotifySafe()
	 * @see DarkMode::setDarkTitleBarEx()
	 * @see DarkMode::setWindowCtlColorSubclass()
	 * @see DarkMode::setChildCtrlsSubclassAndTheme()
	 * @see DarkMode::setDarkWndSafe()
	 */
	void setDarkWndSafeEx(HWND hWnd, bool useWin11Features)
	{
		if (hWnd == nullptr)
		{
			return;
		}

		DarkMode::setDarkTitleBarEx(hWnd, useWin11Features);
		DarkMode::setWindowCtlColorSubclass(hWnd);
		DarkMode::setChildCtrlsSubclassAndTheme(hWnd);
	}

	/**
	 * @brief Applies visual styles; ctl color message and child controls subclassings with Windows 11 features.
	 *
	 * Forwards to `DarkMode::setDarkWndSafeEx` with parameter `useWin11Features` as `true`.
	 *
	 * @param hWnd Handle to the window.
	 *
	 * @see DarkMode::setDarkWndSafeEx()
	 */
	void setDarkWndSafe(HWND hWnd)
	{
		DarkMode::setDarkWndSafeEx(hWnd, true);
	}

	/**
	 * @brief Applies visual styles; ctl color message, child controls, custom drawing, and setting change subclassings to a window safely.
	 *
	 * Ensures the specified window is not `nullptr` and then:
	 * - Enables the dark title bar
	 * - Subclasses the window for control coloring
	 * - Applies theming and subclassing to child controls
	 * - Enables custom draw-based theming via notification subclassing
	 * - Subclasses the window to handle dark mode change if window mode is enabled.
	 *
	 * @param hWnd                      Handle to the window. No action taken if `nullptr`.
	 * @param setSettingChangeSubclass  `true` to set setting change subclass if applicable.
	 * @param useWin11Features          `true` to enable Windows 11 specific styling like Mica or rounded corners.
	 *
	 * @note `setSettingChangeSubclass = true` should be used only on main window.
	 *       For other secondary windows and controls use @ref DarkMode::setDarkWndNotifySafe.
	 *       Should not be used in combination with @ref DarkMode::setDarkWndSafe
	 *       and @ref DarkMode::setDarkWndNotifySafe to avoid overlapping styling logic.
	 *
	 * @see DarkMode::setDarkWndNotifySafe()
	 * @see DarkMode::setDarkWndSafe()
	 * @see DarkMode::setDarkTitleBarEx()
	 * @see DarkMode::setWindowCtlColorSubclass()
	 * @see DarkMode::setWindowNotifyCustomDrawSubclass()
	 * @see DarkMode::setChildCtrlsSubclassAndTheme()
	 * @see DarkMode::isWindowsModeEnabled()
	 * @see DarkMode::setWindowSettingChangeSubclass()
	 */
	void setDarkWndNotifySafeEx(HWND hWnd, bool setSettingChangeSubclass, bool useWin11Features)
	{
		if (hWnd == nullptr)
		{
			return;
		}

		DarkMode::setDarkTitleBarEx(hWnd, useWin11Features);
		DarkMode::setWindowCtlColorSubclass(hWnd);
		DarkMode::setWindowNotifyCustomDrawSubclass(hWnd);
		DarkMode::setChildCtrlsSubclassAndTheme(hWnd);
		if (setSettingChangeSubclass && DarkMode::isWindowsModeEnabled())
		{
			DarkMode::setWindowSettingChangeSubclass(hWnd);
		}
	}

	/**
	 * @brief Applies visual styles; ctl color message, child controls, and custom drawing subclassings with Windows 11 features.
	 *
	 * Calls @ref DarkMode::setDarkWndNotifySafeEx with `setSettingChangeSubclass = false`
	 * and `useWin11Features = true`, streamlining dark mode setup for secondary or transient windows
	 * that don't need to track system dark mode changes.
	 *
	 * @param hWnd Handle to the target window.
	 *
	 * @note Should not be used in combination with @ref DarkMode::setDarkWndSafe
	 *       and @ref DarkMode::setDarkWndNotifySafeEx to avoid overlapping styling logic.
	 *
	 * @see DarkMode::setDarkWndNotifySafeEx()
	 * @see DarkMode::setDarkWndSafe()
	 */
	void setDarkWndNotifySafe(HWND hWnd)
	{
		DarkMode::setDarkWndNotifySafeEx(hWnd, false, true);
	}

	/**
	 * @brief Enables or disables theme-based dialog background textures in classic mode.
	 *
	 * Applies `ETDT_ENABLETAB` only when `theme` is `true` and the current mode is classic.
	 * This replaces the default classic gray background with a lighter themed texture.
	 * Otherwise disables themed dialog textures with `ETDT_DISABLE`.
	 *
	 * @param hWnd  Handle to the target dialog window.
	 * @param theme `true` to enable themed tab textures in classic mode.
	 *
	 * @see EnableThemeDialogTexture
	 */
	void enableThemeDialogTexture(HWND hWnd, bool theme)
	{
		::EnableThemeDialogTexture(hWnd, theme && (g_dmCfg.m_dmType == DarkModeType::classic) ? ETDT_ENABLETAB : ETDT_DISABLE);
	}

	/**
	 * @brief Enables or disables visual styles for a window.
	 *
	 * Applies `SetWindowTheme(hWnd, L"", L"")` when `doDisable` is `true`, effectively removing
	 * the current theme. Restores default theming when `doDisable` is `false`.
	 *
	 * @param hWnd      Handle to the window.
	 * @param doDisable `true` to strip visual styles, `false` to re-enable them.
	 *
	 * @see SetWindowTheme
	 */
	void disableVisualStyle(HWND hWnd, bool doDisable)
	{
		if (doDisable)
		{
			::SetWindowTheme(hWnd, L"", L"");
		}
		else
		{
			::SetWindowTheme(hWnd, nullptr, nullptr);
		}
	}

	/**
	 * @brief Calculates perceptual lightness of a COLORREF color.
	 *
	 * Converts the RGB color to linear space and calculates perceived lightness.
	 *
	 * @param clr COLORREF in 0xBBGGRR format.
	 * @return Lightness value as a double.
	 *
	 * @note Based on: https://stackoverflow.com/a/56678483
	 */
	double calculatePerceivedLightness(COLORREF clr)
	{
		auto linearValue = [](double colorChannel) -> double
		{
			colorChannel /= 255.0;

			static constexpr double treshhold = 0.04045;
			static constexpr double lowScalingFactor = 12.92;
			static constexpr double gammaOffset = 0.055;
			static constexpr double gammaScalingFactor = 1.055;
			static constexpr double gammaExp = 2.4;

			if (colorChannel <= treshhold)
			{
				return colorChannel / lowScalingFactor;
			}
			return std::pow(((colorChannel + gammaOffset) / gammaScalingFactor), gammaExp);
		};

		const double r = linearValue(static_cast<double>(GetRValue(clr)));
		const double g = linearValue(static_cast<double>(GetGValue(clr)));
		const double b = linearValue(static_cast<double>(GetBValue(clr)));

		static constexpr double rWeight = 0.2126;
		static constexpr double gWeight = 0.7152;
		static constexpr double bWeight = 0.0722;

		const double luminance = (rWeight * r) + (gWeight * g) + (bWeight * b);

		static constexpr double cieEpsilon = 216.0 / 24389.0;
		static constexpr double cieKappa = 24389.0 / 27.0;
		static constexpr double oneThird = 1.0 / 3.0;
		static constexpr double scalingFactor = 116.0;
		static constexpr double offset = 16.0;

		// calculate lightness

		if (luminance <= cieEpsilon)
		{
			return (luminance * cieKappa);
		}
		return ((std::pow(luminance, oneThird) * scalingFactor) - offset);
	}

	/**
	 * @brief Retrieves the current TreeView style configuration.
	 *
	 * @return Integer with enum value corresponding to the current `TreeViewStyle`.
	 */
	int getTreeViewStyle()
	{
		return static_cast<int>(g_dmCfg.m_tvStyle);
	}

	/// Set TreeView style
	static void setTreeViewStyle(TreeViewStyle tvStyle)
	{
		g_dmCfg.m_tvStyle = tvStyle;
	}

	/**
	 * @brief Determines appropriate TreeView style based on background perceived lightness.
	 *
	 * Checks the perceived lightness of the current view background and
	 * selects a corresponding style: dark, light, or classic. Style selection
	 * is based on how far the lightness deviates from the middle gray threshold range
	 * around the midpoint value (50.0).
	 *
	 * @see DarkMode::calculatePerceivedLightness()
	 */
	void calculateTreeViewStyle()
	{
		static constexpr double middle = 50.0;
		const COLORREF bgColor = DarkMode::getViewBackgroundColor();

		if (g_dmCfg.m_tvBackground != bgColor || g_dmCfg.m_lightness == middle)
		{
			g_dmCfg.m_lightness = DarkMode::calculatePerceivedLightness(bgColor);
			g_dmCfg.m_tvBackground = bgColor;
		}

		if (g_dmCfg.m_lightness < (middle - kMiddleGrayRange))
		{
			DarkMode::setTreeViewStyle(TreeViewStyle::dark);
		}
		else if (g_dmCfg.m_lightness > (middle + kMiddleGrayRange))
		{
			DarkMode::setTreeViewStyle(TreeViewStyle::light);
		}
		else
		{
			DarkMode::setTreeViewStyle(TreeViewStyle::classic);
		}
	}

	/**
	 * @brief (Re)applies the appropriate window theme style to the specified TreeView .
	 *
	 * Updates the TreeView's visual behavior and theme based on the currently selected
	 * style @ref DarkMode::getTreeViewStyle. It conditionally adjusts the `TVS_TRACKSELECT`
	 * style flag and applies a matching visual theme using `SetWindowTheme()`.
	 *
	 * If `force` is `true`, the style is applied regardless of previous state.
	 * Otherwise, the update occurs only if the style has changed since the last update.
	 *
	 * - `light`: Enables `TVS_TRACKSELECT`, applies "Explorer" theme.
	 * - `dark`: If supported, enables `TVS_TRACKSELECT`, applies "DarkMode_Explorer" theme.
	 * - `classic`: Disables `TVS_TRACKSELECT`, clears the theme.
	 *
	 * @param hWnd  Handle to the TreeView control.
	 * @param force Whether to forcibly reapply the style even if unchanged.
	 *
	 * @see TreeViewStyle
	 * @see DarkMode::getTreeViewStyle()
	 * @see DarkMode::getPrevTreeViewStyle()
	 */
	void setTreeViewWindowThemeEx(HWND hWnd, bool force)
	{
		if (force || DarkMode::getPrevTreeViewStyle() != DarkMode::getTreeViewStyle())
		{
			auto nStyle = ::GetWindowLongPtr(hWnd, GWL_STYLE);
			const bool hasHotStyle = (nStyle & TVS_TRACKSELECT) == TVS_TRACKSELECT;
			bool change = false;
			std::wstring strSubAppName;

			switch (static_cast<TreeViewStyle>(DarkMode::getTreeViewStyle()))
			{
				case TreeViewStyle::light:
				{
					if (!hasHotStyle)
					{
						nStyle |= TVS_TRACKSELECT;
						change = true;
					}
					strSubAppName = L"Explorer";
					break;
				}

				case TreeViewStyle::dark:
				{
					if (DarkMode::isExperimentalSupported())
					{
						if (!hasHotStyle)
						{
							nStyle |= TVS_TRACKSELECT;
							change = true;
						}
						strSubAppName = L"DarkMode_Explorer";
						break;
					}
					[[fallthrough]];
				}

				case TreeViewStyle::classic:
				{
					if (hasHotStyle)
					{
						nStyle &= ~TVS_TRACKSELECT;
						change = true;
					}
					strSubAppName = L"";
					break;
				}
			}

			if (change)
			{
				::SetWindowLongPtr(hWnd, GWL_STYLE, nStyle);
			}

			::SetWindowTheme(hWnd, strSubAppName.empty() ? nullptr : strSubAppName.c_str(), nullptr);
		}
	}

	/**
	 * @brief Applies the appropriate window theme style to the specified TreeView.
	 *
	 * Forwards to `DarkMode::setTreeViewWindowThemeEx` with `force = false` to change tree view style
	 * only if needed.
	 *
	 * @param hWnd  Handle to the TreeView control.
	 *
	 * @see DarkMode::setTreeViewWindowThemeEx()
	 */
	void setTreeViewWindowTheme(HWND hWnd)
	{
		DarkMode::setTreeViewWindowThemeEx(hWnd, false);
	}

	/**
	 * @brief Retrieves the previous TreeView style configuration.
	 *
	 * @return Reference to the previous `TreeViewStyle`.
	 */
	int getPrevTreeViewStyle()
	{
		return static_cast<int>(g_dmCfg.m_tvStylePrev);
	}

	/**
	 * @brief Stores the current TreeView style as the previous style for later comparison.
	 */
	void setPrevTreeViewStyle()
	{
		g_dmCfg.m_tvStylePrev = static_cast<TreeViewStyle>(DarkMode::getTreeViewStyle());
	}

	/**
	 * @brief Checks whether the current theme is dark.
	 *
	 * Internally it use TreeView style to determine if dark theme is used.
	 *
	 * @return `true` if the active style is `TreeViewStyle::dark`, otherwise `false`.
	 *
	 * @see DarkMode::getTreeViewStyle()
	 */
	bool isThemeDark()
	{
		return static_cast<TreeViewStyle>(DarkMode::getTreeViewStyle()) == TreeViewStyle::dark;
	}

	/**
	 * @brief Checks whether the color is dark.
	 *
	 * @param clr Color to check.
	 *
	 * @return `true` if the perceived lightness of the color
	 *         is less than (50.0 - kMiddleGrayRange), otherwise `false`.
	 *
	 * @see DarkMode::calculatePerceivedLightness()
	 */
	bool isColorDark(COLORREF clr)
	{
		static constexpr double middle = 50.0;
		return DarkMode::calculatePerceivedLightness(clr) < (middle - kMiddleGrayRange);
	}

	/**
	 * @brief Forces a window to redraw its non-client frame.
	 *
	 * Triggers a non-client area update by using `SWP_FRAMECHANGED` without changing
	 * size, position, or Z-order.
	 *
	 * @param hWnd Handle to the target window.
	 */
	void redrawWindowFrame(HWND hWnd)
	{
		::SetWindowPos(hWnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
	}

	/**
	 * @brief Sets or clears a specific window style or extended style.
	 *
	 * Checks if the specified `dwFlag` is already set and toggles it if needed.
	 * Only valid for `GWL_STYLE` or `GWL_EXSTYLE`.
	 *
	 * @param hWnd      Handle to the window.
	 * @param setFlag   `true` to set the flag, `false` to clear it.
	 * @param dwFlag    Style bitmask to apply.
	 * @param gwlIdx    Either `GWL_STYLE` or `GWL_EXSTYLE`.
	 * @return `TRUE` if modified, `FALSE` if unchanged, `-1` if invalid index.
	 */
	static int setWindowLongPtrStyle(HWND hWnd, bool setFlag, LONG_PTR dwFlag, int gwlIdx)
	{
		if ((gwlIdx != GWL_STYLE) && (gwlIdx != GWL_EXSTYLE))
		{
			return -1;
		}

		auto nStyle = ::GetWindowLongPtr(hWnd, gwlIdx);
		const bool hasFlag = (nStyle & dwFlag) == dwFlag;

		if (setFlag != hasFlag)
		{
			nStyle ^= dwFlag;
			::SetWindowLongPtr(hWnd, gwlIdx, nStyle);
			return TRUE;
		}
		return FALSE;
	}

	/**
	 * @brief Sets a window's standard style flags and redraws window if needed.
	 *
	 * Wraps @ref DarkMode::setWindowLongPtrStyle with `GWL_STYLE`
	 * and calls @ref DarkMode::redrawWindowFrame if a change occurs.
	 *
	 * @param hWnd      Handle to the target window.
	 * @param setStyle  `true` to set the flag, `false` to remove it.
	 * @param styleFlag Style bit to modify.
	 */
	void setWindowStyle(HWND hWnd, bool setStyle, LONG_PTR styleFlag)
	{
		if (DarkMode::setWindowLongPtrStyle(hWnd, setStyle, styleFlag, GWL_STYLE) == TRUE)
		{
			DarkMode::redrawWindowFrame(hWnd);
		}
	}

	/**
	 * @brief Sets a window's extended style flags and redraws window if needed.
	 *
	 * Wraps @ref DarkMode::setWindowLongPtrStyle with `GWL_EXSTYLE`
	 * and calls @ref DarkMode::redrawWindowFrame if a change occurs.
	 *
	 * @param hWnd          Handle to the target window.
	 * @param setExStyle    `true` to set the flag, `false` to remove it.
	 * @param exStyleFlag   Extended style bit to modify.
	 */
	void setWindowExStyle(HWND hWnd, bool setExStyle, LONG_PTR exStyleFlag)
	{
		if (DarkMode::setWindowLongPtrStyle(hWnd, setExStyle, exStyleFlag, GWL_EXSTYLE) == TRUE)
		{
			DarkMode::redrawWindowFrame(hWnd);
		}
	}

	/**
	 * @brief Replaces an extended edge (e.g. client edge) with a standard window border.
	 *
	 * The given `exStyleFlag` must be a valid edge-related extended window style:
	 * - `WS_EX_CLIENTEDGE`
	 * - `WS_EX_DLGMODALFRAME`
	 * - `WS_EX_STATICEDGE`
	 * - `WS_EX_WINDOWEDGE`
	 * ...or any combination of these.
	 *
	 * If `replace` is `true`, the specified extended edge style(s) are removed and
	 * `WS_BORDER` is applied. If `false`, the edge style(s) are restored and `WS_BORDER` is cleared.
	 *
	 * @param hWnd          Handle to the target window.
	 * @param replace       `true` to apply standard border; `false` to restore extended edge(s).
	 * @param exStyleFlag   One or more valid edge-related extended styles.
	 *
	 * @see DarkMode::setWindowExStyle()
	 * @see DarkMode::setWindowStyle()
	 */
	void replaceExEdgeWithBorder(HWND hWnd, bool replace, LONG_PTR exStyleFlag)
	{
		DarkMode::setWindowExStyle(hWnd, !replace, exStyleFlag);
		DarkMode::setWindowStyle(hWnd, replace, WS_BORDER);
	}

	/**
	 * @brief Safely toggles `WS_EX_CLIENTEDGE` with `WS_BORDER` based on dark mode state.
	 *
	 * If dark mode is enabled, removes `WS_EX_CLIENTEDGE` and applies `WS_BORDER`.
	 * Otherwise restores the extended edge style.
	 *
	 * @param hWnd Handle to the target window. No action is taken if `hWnd` is `nullptr`.
	 *
	 * @see DarkMode::replaceExEdgeWithBorder()
	 */
	void replaceClientEdgeWithBorderSafe(HWND hWnd)
	{
		if (hWnd != nullptr)
		{
			DarkMode::replaceExEdgeWithBorder(hWnd, DarkMode::isEnabled(), WS_EX_CLIENTEDGE);
		}
	}

	/**
	 * @brief Applies classic-themed styling to a progress bar in non-classic mode.
	 *
	 * When dark mode is enabled, applies `WS_BORDER`, removes visual styles
	 * to allow to set custom background and fill colors using:
	 * - Background: `DarkMode::getBackgroundColor()`
	 * - Fill: Hardcoded green `0x06B025` via `PBM_SETBARCOLOR`
	 *
	 * Typically used for marquee style progress bar.
	 *
	 * @param hWnd Handle to the progress bar control.
	 *
	 * @see DarkMode::setWindowStyle()
	 * @see DarkMode::disableVisualStyle()
	 */
	void setProgressBarClassicTheme(HWND hWnd)
	{
		DarkMode::setWindowStyle(hWnd, DarkMode::isEnabled(), WS_BORDER);
		DarkMode::disableVisualStyle(hWnd, DarkMode::isEnabled());
		DarkMode::setWindowExStyle(hWnd, false, WS_EX_STATICEDGE);
		if (DarkMode::isEnabled())
		{
			::SendMessage(hWnd, PBM_SETBKCOLOR, 0, static_cast<LPARAM>(DarkMode::getCtrlBackgroundColor()));
			static constexpr COLORREF greenLight = dmlib_color::HEXRGB(0x06B025);
			static constexpr COLORREF greenDark = dmlib_color::HEXRGB(0x0F7B0F);
			::SendMessage(hWnd, PBM_SETBARCOLOR, 0, static_cast<LPARAM>(DarkMode::isExperimentalActive() ? greenDark : greenLight));
		}
	}

	/**
	 * @brief Handles text and background colorizing for read-only controls.
	 *
	 * Sets the text color and background color on the provided HDC.
	 * Returns the corresponding background brush for painting.
	 * Typically used for read-only controls (e.g. edit control and combo box' list box).
	 * Typically used in response to `WM_CTLCOLORSTATIC` or in `WM_CTLCOLORLISTBOX`
	 * via @ref DarkMode::onCtlColorListbox
	 *
	 * @param hdc Handle to the device context (HDC) receiving the drawing instructions.
	 * @return Background brush to use for painting, or `FALSE` (0) if classic mode is enabled
	 *         and `_DARKMODELIB_DLG_PROC_CTLCOLOR_RETURNS` is defined
	 *         and has non-zero unsigned value.
	 *
	 * @see DarkMode::WindowCtlColorSubclass()
	 * @see DarkMode::onCtlColorListbox()
	 */
	LRESULT onCtlColor(HDC hdc)
	{
#if defined(_DARKMODELIB_DLG_PROC_CTLCOLOR_RETURNS) && (_DARKMODELIB_DLG_PROC_CTLCOLOR_RETURNS > 0)
		if (!DarkMode::_isEnabled())
		{
			return FALSE;
		}
#endif
		::SetTextColor(hdc, DarkMode::getTextColor());
		::SetBkColor(hdc, DarkMode::getBackgroundColor());
		return reinterpret_cast<LRESULT>(DarkMode::getBackgroundBrush());
	}

	/**
	 * @brief Handles text and background colorizing for interactive controls.
	 *
	 * Sets the text and background colors on the provided HDC.
	 * Returns the corresponding brush used to paint the background.
	 * Typically used in response to `WM_CTLCOLOREDIT` and `WM_CTLCOLORLISTBOX`
	 * via @ref DarkMode::onCtlColorListbox
	 *
	 * @param hdc Handle to the device context for the target control.
	 * @return The background brush, or `FALSE` if dark mode is disabled and
	 *         `_DARKMODELIB_DLG_PROC_CTLCOLOR_RETURNS` is defined
	 *         and has non-zero unsigned value.
	 *
	 * @see DarkMode::WindowCtlColorSubclass()
	 * @see DarkMode::onCtlColorListbox()
	 */
	LRESULT onCtlColorCtrl(HDC hdc)
	{
#if defined(_DARKMODELIB_DLG_PROC_CTLCOLOR_RETURNS) && (_DARKMODELIB_DLG_PROC_CTLCOLOR_RETURNS > 0)
		if (!DarkMode::_isEnabled())
		{
			return FALSE;
		}
#endif

		::SetTextColor(hdc, DarkMode::getTextColor());
		::SetBkColor(hdc, DarkMode::getCtrlBackgroundColor());
		return reinterpret_cast<LRESULT>(DarkMode::getCtrlBackgroundBrush());
	}

	/**
	 * @brief Handles text and background colorizing for window and disabled non-text controls.
	 *
	 * Sets the text and background colors on the provided HDC.
	 * Returns the corresponding brush used to paint the background.
	 * Typically used in response to `WM_CTLCOLORDLG`, `WM_CTLCOLORSTATIC`
	 * and `WM_CTLCOLORLISTBOX` via @ref DarkMode::onCtlColorListbox
	 *
	 * @param hdc Handle to the device context for the target control.
	 * @return The background brush, or `FALSE` if dark mode is disabled and
	 *         `_DARKMODELIB_DLG_PROC_CTLCOLOR_RETURNS` is defined
	 *         and has non-zero unsigned value.
	 *
	 * @see DarkMode::WindowCtlColorSubclass()
	 * @see DarkMode::onCtlColorListbox()
	 */
	LRESULT onCtlColorDlg(HDC hdc)
	{
#if defined(_DARKMODELIB_DLG_PROC_CTLCOLOR_RETURNS) && (_DARKMODELIB_DLG_PROC_CTLCOLOR_RETURNS > 0)
		if (!DarkMode::_isEnabled())
		{
			return FALSE;
		}
#endif

		::SetTextColor(hdc, DarkMode::getTextColor());
		::SetBkColor(hdc, DarkMode::getDlgBackgroundColor());
		return reinterpret_cast<LRESULT>(DarkMode::getDlgBackgroundBrush());
	}

	/**
	 * @brief Handles text and background colorizing for error state (for specific usage).
	 *
	 * Sets the text and background colors on the provided HDC.
	 *
	 * @param hdc Handle to the device context for the target control.
	 * @return The background brush, or `FALSE` if dark mode is disabled and
	 *         `_DARKMODELIB_DLG_PROC_CTLCOLOR_RETURNS` is defined
	 *         and has non-zero unsigned value.
	 *
	 * @see DarkMode::WindowCtlColorSubclass()
	 */
	LRESULT onCtlColorError(HDC hdc)
	{
#if defined(_DARKMODELIB_DLG_PROC_CTLCOLOR_RETURNS) && (_DARKMODELIB_DLG_PROC_CTLCOLOR_RETURNS > 0)
		if (!DarkMode::_isEnabled())
		{
			return FALSE;
		}
#endif

		::SetTextColor(hdc, DarkMode::getTextColor());
		::SetBkColor(hdc, DarkMode::getErrorBackgroundColor());
		return reinterpret_cast<LRESULT>(DarkMode::getErrorBackgroundBrush());
	}

	/**
	 * @brief Handles text and background colorizing for static text controls.
	 *
	 * Sets the text and background colors on the provided HDC.
	 * Colors depend on if control is enabled.
	 * Returns the corresponding brush used to paint the background.
	 * Typically used in response to `WM_CTLCOLORSTATIC`.
	 *
	 * @param hdc           Handle to the device context for the target control.
	 * @param isTextEnabled `true` if control should use enabled colors.
	 * @return The background brush, or `FALSE` if dark mode is disabled and
	 *         `_DARKMODELIB_DLG_PROC_CTLCOLOR_RETURNS` is defined
	 *         and has non-zero unsigned value.
	 *
	 * @see DarkMode::WindowCtlColorSubclass()
	 */
	LRESULT onCtlColorDlgStaticText(HDC hdc, bool isTextEnabled)
	{
#if defined(_DARKMODELIB_DLG_PROC_CTLCOLOR_RETURNS) && (_DARKMODELIB_DLG_PROC_CTLCOLOR_RETURNS > 0)
		if (!DarkMode::_isEnabled())
		{
			::SetTextColor(hdc, ::GetSysColor(isTextEnabled ? COLOR_WINDOWTEXT : COLOR_GRAYTEXT));
			return FALSE;
		}
#endif
		::SetTextColor(hdc, isTextEnabled ? DarkMode::getTextColor() : DarkMode::getDisabledTextColor());
		::SetBkColor(hdc, DarkMode::getDlgBackgroundColor());
		return reinterpret_cast<LRESULT>(DarkMode::getDlgBackgroundBrush());
	}

	/**
	 * @brief Handles text and background colorizing for SysLink controls.
	 *
	 * Sets the text and background colors on the provided HDC.
	 * Colors depend on if control is enabled.
	 * Returns the corresponding brush used to paint the background.
	 * Typically used in response to `WM_CTLCOLORSTATIC`.
	 *
	 * @param hdc           Handle to the device context for the target control.
	 * @param isTextEnabled `true` if control should use enabled colors.
	 * @return The background brush, or `FALSE` if dark mode is disabled and
	 *         `_DARKMODELIB_DLG_PROC_CTLCOLOR_RETURNS` is defined
	 *         and has non-zero unsigned value.
	 *
	 * @see DarkMode::WindowCtlColorSubclass()
	 */
	LRESULT onCtlColorDlgLinkText(HDC hdc, bool isTextEnabled)
	{
#if defined(_DARKMODELIB_DLG_PROC_CTLCOLOR_RETURNS) && (_DARKMODELIB_DLG_PROC_CTLCOLOR_RETURNS > 0)
		if (!DarkMode::_isEnabled())
		{
			::SetTextColor(hdc, ::GetSysColor(isTextEnabled ? COLOR_HOTLIGHT : COLOR_GRAYTEXT));
			return FALSE;
		}
#endif
		::SetTextColor(hdc, isTextEnabled ? DarkMode::getLinkTextColor() : DarkMode::getDisabledTextColor());
		::SetBkColor(hdc, DarkMode::getDlgBackgroundColor());
		return reinterpret_cast<LRESULT>(DarkMode::getDlgBackgroundBrush());
	}

	/**
	 * @brief Handles text and background colorizing for list box controls.
	 *
	 * Inspects the list box style flags to detect if it's part of a combo box (via `LBS_COMBOBOX`)
	 * and whether experimental feature is active. Based on the context, delegates to:
	 * - @ref DarkMode::onCtlColorCtrl for standard enabled listboxes
	 * - @ref DarkMode::onCtlColorDlg for disabled ones or when dark mode is disabled
	 * - @ref DarkMode::onCtlColor for combo box' listbox
	 *
	 * @param wParam    WPARAM from `WM_CTLCOLORLISTBOX`, representing the HDC.
	 * @param lParam    LPARAM from `WM_CTLCOLORLISTBOX`, representing the HWND of the listbox.
	 * @return The brush handle as LRESULT for background painting, or `FALSE` if not themed.
	 *
	 * @see DarkMode::WindowCtlColorSubclass()
	 * @see DarkMode::onCtlColor()
	 * @see DarkMode::onCtlColorCtrl()
	 * @see DarkMode::onCtlColorDlg()
	 */
	LRESULT onCtlColorListbox(WPARAM wParam, LPARAM lParam)
	{
		auto hdc = reinterpret_cast<HDC>(wParam);
		auto hWnd = reinterpret_cast<HWND>(lParam);

		const auto nStyle = ::GetWindowLongPtr(hWnd, GWL_STYLE);
		const bool isComboBox = (nStyle & LBS_COMBOBOX) == LBS_COMBOBOX;
		if ((!isComboBox || !DarkMode::isExperimentalActive()))
		{
			if (::IsWindowEnabled(hWnd) == TRUE)
			{
				return DarkMode::onCtlColorCtrl(hdc);
			}
			return DarkMode::onCtlColorDlg(hdc);
		}
		return DarkMode::onCtlColor(hdc);
	}

	/**
	 * @brief Hook procedure for customizing common dialogs with custom colors.
	 */
	UINT_PTR CALLBACK HookDlgProc(HWND hWnd, UINT uMsg, [[maybe_unused]] WPARAM wParam, [[maybe_unused]] LPARAM lParam)
	{
		if (uMsg == WM_INITDIALOG)
		{
			DarkMode::setDarkWndSafe(hWnd);
			return TRUE;
		}
		return FALSE;
	}
	
	/**
	 * @brief Applies dark mode visual styles to task dialog.
	 *
	 * @note Currently colors cannot be customized.
	 *
	 * @param[in] hWnd Handle to the task dialog.
	 *
	 * @see dmlib_subclass::setTaskDlgChildCtrlsSubclassAndTheme()
	 */
	void setDarkTaskDlg(HWND hWnd)
	{
		if (DarkMode::isExperimentalActive())
		{
			DarkMode::setDarkTitleBar(hWnd);
			DarkMode::setDarkExplorerTheme(hWnd);
			dmlib_subclass::setTaskDlgChildCtrlsSubclassAndTheme(hWnd);
		}
	}

	/**
	 * @brief Simple task dialog callback procedure to enable dark mode support.
	 *
	 * @param[in]   hWnd        Handle to the task dialog.
	 * @param[in]   uMsg        Message identifier.
	 * @param[in]   wParam      First message parameter (unused).
	 * @param[in]   lParam      Second message parameter (unused).
	 * @param[in]   lpRefData   Reserved data (unused).
	 * @return HRESULT A value defined by the hook procedure.
	 *
	 * @see DarkMode::setDarkTaskDlg()
	 * @see DarkMode::darkTaskDialogIndirect()
	 */
	HRESULT CALLBACK DarkTaskDlgCallback(
		HWND hWnd,
		UINT uMsg,
		[[maybe_unused]] WPARAM wParam,
		[[maybe_unused]] LPARAM lParam,
		[[maybe_unused]] LONG_PTR lpRefData
	)
	{
		if (uMsg == TDN_DIALOG_CONSTRUCTED)
		{
			DarkMode::setDarkTaskDlg(hWnd);
		}
		return S_OK;
	}

	/**
	 * @brief Wrapper for `TaskDialogIndirect` with dark mode support.
	 */
	HRESULT darkTaskDialogIndirect(
		const TASKDIALOGCONFIG* pTaskConfig,
		int* pnButton,
		int* pnRadioButton,
		BOOL* pfVerificationFlagChecked
	)
	{
		dmlib_hook::hookThemeColor();
		const HRESULT retVal = ::TaskDialogIndirect(pTaskConfig, pnButton, pnRadioButton, pfVerificationFlagChecked);
		dmlib_hook::unhookThemeColor();
		return retVal;
	}
} // namespace DarkMode

#endif // !defined(_DARKMODELIB_NOT_USED)
