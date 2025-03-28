﻿// Copyright (C)2024 ozone10

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.


// Based on Notepad++ dark mode code, original by adzm / Adam D. Walling
// with modification from Notepad++ team.
// Heavily modified by ozone10 (contributor of Notepad++)

#include "StdAfx.h"

#include "DarkModeSubclass.h"

#include "DarkMode.h"
#include "UAHMenuBar.h"

#include <dwmapi.h>
#include <uxtheme.h>
#include <vssym32.h>
#include <windowsx.h>

#include <shlwapi.h>

#include <array>
#include <cmath>
#include <string>

#ifdef __GNUC__
#define WINAPI_LAMBDA WINAPI
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#else
#define WINAPI_LAMBDA
#endif

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif

//#ifndef WM_DPICHANGED_BEFOREPARENT
//#define WM_DPICHANGED_BEFOREPARENT 0x02E2
//#endif
//
//#ifndef WM_DPICHANGED_AFTERPARENT
//#define WM_DPICHANGED_AFTERPARENT 0x02E3
//#endif
//
//#ifndef WM_GETDPISCALEDSIZE
//#define WM_GETDPISCALEDSIZE 0x02E4
//#endif

#ifdef _MSC_VER
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Gdi32.lib")
#endif

static constexpr COLORREF HEXRGB(DWORD rrggbb) {
	// from 0xRRGGBB like natural #RRGGBB
	// to the little-endian 0xBBGGRR
	return
		((rrggbb & 0xFF0000) >> 16) |
		((rrggbb & 0x00FF00)) |
		((rrggbb & 0x0000FF) << 16);
}

static std::wstring getWndClassName(HWND hWnd)
{
	constexpr int strLen = 32;
	std::wstring className(strLen, 0);
	className.resize(::GetClassName(hWnd, &className[0], strLen));
	return className;
}

static bool cmpWndClassName(HWND hWnd, std::wstring classNameToCmp)
{
	return getWndClassName(hWnd) == classNameToCmp;
}

static std::wstring getIniPath(std::wstring iniFilename)
{
	wchar_t buffer[MAX_PATH]{};
	::GetModuleFileName(nullptr, buffer, MAX_PATH);
	
	wchar_t* lastSlash = wcsrchr(buffer, L'\\');
	if (lastSlash)
	{
		*lastSlash = L'\0';
		std::wstring iniPath(buffer);

		iniPath += L"\\";
		iniPath += iniFilename;
		iniPath += L".ini";
		return iniPath;
	}
	return L"";
}

static bool fileExists(std::wstring filePath)
{
	DWORD dwAttrib = ::GetFileAttributes(filePath.c_str());
	return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

static bool setClrFromIni(std::wstring sectionName, std::wstring keyName, std::wstring iniFilePath, COLORREF* clr)
{
	constexpr int maxStringLength = 7;
	wchar_t buffer[maxStringLength + 1]{};
	
	::GetPrivateProfileString(sectionName.c_str(), keyName.c_str(), L"", buffer, maxStringLength, iniFilePath.c_str());

	size_t length = wcslen(buffer);
	if (length != maxStringLength - 1)
	{
		return false;
	}

	for (size_t i = 0; i < length; ++i)
	{
		if (!iswxdigit(buffer[i]))
		{
			return false;
		}
	}

	COLORREF clrTmp{ *clr };

	try
	{
		clrTmp = HEXRGB(std::stoi(buffer, nullptr, 16));
	}
	catch (const std::exception&)
	{
		return false;
	}

	*clr = clrTmp;
	return true;
}

namespace DarkMode
{
	struct Brushes
	{
		HBRUSH background = nullptr;
		HBRUSH controlBackground = nullptr;
		HBRUSH hotBackground = nullptr;
		HBRUSH dlgBackground = nullptr;
		HBRUSH errorBackground = nullptr;

		HBRUSH edge = nullptr;
		HBRUSH hotEdge = nullptr;
		HBRUSH disabledEdge = nullptr;

		Brushes(const Colors& colors)
			: background(::CreateSolidBrush(colors.background))
			, controlBackground(::CreateSolidBrush(colors.controlBackground))
			, hotBackground(::CreateSolidBrush(colors.hotBackground))
			, dlgBackground(::CreateSolidBrush(colors.dlgBackground))
			, errorBackground(::CreateSolidBrush(colors.errorBackground))

			, edge(::CreateSolidBrush(colors.edge))
			, hotEdge(::CreateSolidBrush(colors.hotEdge))
			, disabledEdge(::CreateSolidBrush(colors.disabledEdge))
		{}

		~Brushes()
		{
			::DeleteObject(background);         background = nullptr;
			::DeleteObject(controlBackground);  controlBackground = nullptr;
			::DeleteObject(hotBackground);      hotBackground = nullptr;
			::DeleteObject(dlgBackground);     dlgBackground = nullptr;
			::DeleteObject(errorBackground);    errorBackground = nullptr;

			::DeleteObject(edge);          edge = nullptr;
			::DeleteObject(hotEdge);       hotEdge = nullptr;
			::DeleteObject(disabledEdge);  disabledEdge = nullptr;
		}

		void change(const Colors& colors)
		{
			::DeleteObject(background);
			::DeleteObject(controlBackground);
			::DeleteObject(hotBackground);
			::DeleteObject(dlgBackground);
			::DeleteObject(errorBackground);

			::DeleteObject(edge);
			::DeleteObject(hotEdge);
			::DeleteObject(disabledEdge);

			background = ::CreateSolidBrush(colors.background);
			controlBackground = ::CreateSolidBrush(colors.controlBackground);
			hotBackground = ::CreateSolidBrush(colors.hotBackground);
			dlgBackground = ::CreateSolidBrush(colors.dlgBackground);
			errorBackground = ::CreateSolidBrush(colors.errorBackground);

			edge = ::CreateSolidBrush(colors.edge);
			hotEdge = ::CreateSolidBrush(colors.hotEdge);
			disabledEdge = ::CreateSolidBrush(colors.disabledEdge);
		}
	};

	struct Pens
	{
		HPEN darkerTextPen = nullptr;
		HPEN edgePen = nullptr;
		HPEN hotEdgePen = nullptr;
		HPEN disabledEdgePen = nullptr;

		Pens(const Colors& colors)
			: darkerTextPen(::CreatePen(PS_SOLID, 1, colors.darkerText))
			, edgePen(::CreatePen(PS_SOLID, 1, colors.edge))
			, hotEdgePen(::CreatePen(PS_SOLID, 1, colors.hotEdge))
			, disabledEdgePen(::CreatePen(PS_SOLID, 1, colors.disabledEdge))
		{}

		~Pens()
		{
			::DeleteObject(darkerTextPen);      darkerTextPen = nullptr;
			::DeleteObject(edgePen);            edgePen = nullptr;
			::DeleteObject(hotEdgePen);         hotEdgePen = nullptr;
			::DeleteObject(disabledEdgePen);    disabledEdgePen = nullptr;
		}

		void change(const Colors& colors)
		{
			::DeleteObject(darkerTextPen);
			::DeleteObject(edgePen);
			::DeleteObject(hotEdgePen);
			::DeleteObject(disabledEdgePen);

			darkerTextPen = ::CreatePen(PS_SOLID, 1, colors.darkerText);
			edgePen = ::CreatePen(PS_SOLID, 1, colors.edge);
			hotEdgePen = ::CreatePen(PS_SOLID, 1, colors.hotEdge);
			disabledEdgePen = ::CreatePen(PS_SOLID, 1, colors.disabledEdge);
		}

	};

	// black (default)
	static const Colors darkColors{
		HEXRGB(0x202020),   // background
		HEXRGB(0x383838),   // controlBackground
		HEXRGB(0x454545),   // hotBackground
		HEXRGB(0x202020),   // dlgBackground
		HEXRGB(0xB00000),   // errorBackground
		HEXRGB(0xE0E0E0),   // textColor
		HEXRGB(0xC0C0C0),   // darkerTextColor
		HEXRGB(0x808080),   // disabledTextColor
		HEXRGB(0xFFFF00),   // linkTextColor
		HEXRGB(0x646464),   // edgeColor
		HEXRGB(0x9B9B9B),   // hotEdgeColor
		HEXRGB(0x484848)    // disabledEdgeColor
	};

	// red tone
	static const Colors darkRedColors{
		HEXRGB(0x302020),   // background
		HEXRGB(0x483838),   // controlBackground
		HEXRGB(0x554545),   // hotBackground
		HEXRGB(0x302020),   // dlgBackground
		HEXRGB(0xC00000),   // errorBackground
		HEXRGB(0xE0E0E0),   // textColor
		HEXRGB(0xC0C0C0),   // darkerTextColor
		HEXRGB(0x808080),   // disabledTextColor
		HEXRGB(0xFFFF00),   // linkTextColor
		HEXRGB(0x908080),   // edgeColor
		HEXRGB(0xBBABAB),   // hotEdgeColor
		HEXRGB(0x584848)    // disabledEdgeColor
	};

	// green tone
	static const Colors darkGreenColors{
		HEXRGB(0x203020),   // background
		HEXRGB(0x384838),   // controlBackground
		HEXRGB(0x455545),   // hotBackground
		HEXRGB(0x203020),   // dlgBackground
		HEXRGB(0xB01000),   // errorBackground
		HEXRGB(0xE0E0E0),   // textColor
		HEXRGB(0xC0C0C0),   // darkerTextColor
		HEXRGB(0x808080),   // disabledTextColor
		HEXRGB(0xFFFF00),   // linkTextColor
		HEXRGB(0x809080),   // edgeColor
		HEXRGB(0xABBBAB),   // hotEdgeColor
		HEXRGB(0x485848)    // disabledEdgeColor
	};

	// blue tone
	static const Colors darkBlueColors{
		HEXRGB(0x202040),   // background
		HEXRGB(0x383858),   // controlBackground
		HEXRGB(0x454565),   // hotBackground
		HEXRGB(0x202040),   // dlgBackground
		HEXRGB(0xB00020),   // errorBackground
		HEXRGB(0xE0E0E0),   // textColor
		HEXRGB(0xC0C0C0),   // darkerTextColor
		HEXRGB(0x808080),   // disabledTextColor
		HEXRGB(0xFFFF00),   // linkTextColor
		HEXRGB(0x8080A0),   // edgeColor
		HEXRGB(0xABABCB),   // hotEdgeColor
		HEXRGB(0x484868)    // disabledEdgeColor
	};

	// purple tone
	static const Colors darkPurpleColors{
		HEXRGB(0x302040),   // background
		HEXRGB(0x483858),   // controlBackground
		HEXRGB(0x554565),   // hotBackground
		HEXRGB(0x302040),   // dlgBackground
		HEXRGB(0xC00020),   // errorBackground
		HEXRGB(0xE0E0E0),   // textColor
		HEXRGB(0xC0C0C0),   // darkerTextColor
		HEXRGB(0x808080),   // disabledTextColor
		HEXRGB(0xFFFF00),   // linkTextColor
		HEXRGB(0x9080A0),   // edgeColor
		HEXRGB(0xBBABCB),   // hotEdgeColor
		HEXRGB(0x584868)    // disabledEdgeColor
	};

	// cyan tone
	static const Colors darkCyanColors{
		HEXRGB(0x203040),   // background
		HEXRGB(0x384858),   // controlBackground
		HEXRGB(0x455565),   // hotBackground
		HEXRGB(0x203040),   // dlgBackground
		HEXRGB(0xB01020),   // errorBackground
		HEXRGB(0xE0E0E0),   // textColor
		HEXRGB(0xC0C0C0),   // darkerTextColor
		HEXRGB(0x808080),   // disabledTextColor
		HEXRGB(0xFFFF00),   // linkTextColor
		HEXRGB(0x8090A0),   // edgeColor
		HEXRGB(0xBBBBCB),   // hotEdgeColor
		HEXRGB(0x485868)    // disabledEdgeColor
	};

	// olive tone
	static const Colors darkOliveColors{
		HEXRGB(0x303020),   // background
		HEXRGB(0x484838),   // controlBackground
		HEXRGB(0x555545),   // hotBackground
		HEXRGB(0x303020),   // dlgBackground
		HEXRGB(0xC01000),   // errorBackground
		HEXRGB(0xE0E0E0),   // textColor
		HEXRGB(0xC0C0C0),   // darkerTextColor
		HEXRGB(0x808080),   // disabledTextColor
		HEXRGB(0xFFFF00),   // linkTextColor
		HEXRGB(0x909080),   // edgeColor
		HEXRGB(0xBBBBAB),   // hotEdgeColor
		HEXRGB(0x585848)    // disabledEdgeColor
	};

	// customized
	static Colors darkCustomizedColors{ darkColors };

	static Colors lightColors{
		::GetSysColor(COLOR_3DFACE),        // background
		::GetSysColor(COLOR_WINDOW),        // controlBackground
		HEXRGB(0xC0DCF3),                   // hotBackground
		::GetSysColor(COLOR_3DFACE),        // dlgBackground
		HEXRGB(0xA01000),                   // errorBackground
		::GetSysColor(COLOR_WINDOWTEXT),    // textColor
		::GetSysColor(COLOR_BTNTEXT),       // darkerTextColor
		::GetSysColor(COLOR_GRAYTEXT),      // disabledTextColor
		::GetSysColor(COLOR_HOTLIGHT),      // linkTextColor
		HEXRGB(0x8D8D8D),                   // edgeColor
		::GetSysColor(COLOR_HIGHLIGHT),     // hotEdgeColor
		::GetSysColor(COLOR_GRAYTEXT)       // disabledEdgeColor
	};

	static ColorTone g_colorToneChoice = DarkMode::ColorTone::blackTone;

	void setDarkCustomColors(ColorTone colorTone)
	{
		switch (colorTone)
		{
			case DarkMode::ColorTone::redTone:
			{
				darkCustomizedColors = darkRedColors;
				break;
			}

			case DarkMode::ColorTone::greenTone:
			{
				darkCustomizedColors = darkGreenColors;
				break;
			}

			case DarkMode::ColorTone::blueTone:
			{
				darkCustomizedColors = darkBlueColors;
				break;
			}

			case DarkMode::ColorTone::purpleTone:
			{
				darkCustomizedColors = darkPurpleColors;
				break;
			}

			case DarkMode::ColorTone::cyanTone:
			{
				darkCustomizedColors = darkCyanColors;
				break;
			}

			case DarkMode::ColorTone::oliveTone:
			{
				darkCustomizedColors = darkOliveColors;
				break;
			}

			case DarkMode::ColorTone::customizedTone:
			case DarkMode::ColorTone::blackTone:
			{
				darkCustomizedColors = darkColors;
				break;
			}
		}
	}

	struct Theme
	{
		Colors _colors;
		Brushes _brushes;
		Pens _pens;

		Theme(const Colors& colors)
			: _colors(colors)
			, _brushes(colors)
			, _pens(colors)
		{}

		void change(const Colors& colors)
		{
			_colors = colors;
			_brushes.change(colors);
			_pens.change(colors);
		}
	};

	Theme tDefault(darkCustomizedColors);

	static ColorsView darkColorsView{
		RGB(41, 49, 52),      // background
		RGB(224, 226, 228),   // text
		RGB(100, 100, 100),   // gridlines
		HEXRGB(0x202020),     // header background
		HEXRGB(0x454545),     // header hot background
		HEXRGB(0xC0C0C0),     // header text
		RGB(100, 100, 100)    // header divider
	};

	static ColorsView lightColorsView{
		RGB(255, 255, 255),   // background
		RGB(0, 0, 0),         // text
		RGB(240, 240, 240),   // gridlines
		RGB(255, 255, 255),   // header background
		RGB(217, 235, 249),   // header hot background
		RGB(0, 0, 0),         // header text
		RGB(229, 229, 229)    // header divider
	};

	static Theme& getTheme()
	{
		return tDefault;
	}

	struct BrushesAndPensView
	{
		HBRUSH background = nullptr;
		HBRUSH gridlines = nullptr;
		HBRUSH headerBackground = nullptr;
		HBRUSH headerHotBackground = nullptr;

		HPEN edge = nullptr;

		BrushesAndPensView(const ColorsView& colors)
			: background(::CreateSolidBrush(colors.background))
			, gridlines(::CreateSolidBrush(colors.gridlines))
			, headerBackground(::CreateSolidBrush(colors.headerBackground))
			, headerHotBackground(::CreateSolidBrush(colors.headerHotBackground))

			, edge(::CreatePen(PS_SOLID, 1, colors.headerEdge))
		{}

		~BrushesAndPensView()
		{
			::DeleteObject(background);             background = nullptr;
			::DeleteObject(gridlines);              gridlines = nullptr;
			::DeleteObject(headerBackground);       headerBackground = nullptr;
			::DeleteObject(headerHotBackground);    headerHotBackground = nullptr;

			::DeleteObject(edge);                   edge = nullptr;
		}

		void change(const ColorsView& colors)
		{
			::DeleteObject(background);
			::DeleteObject(gridlines);
			::DeleteObject(headerBackground);
			::DeleteObject(headerHotBackground);

			background = ::CreateSolidBrush(colors.background);
			gridlines = ::CreateSolidBrush(colors.gridlines);
			headerBackground = ::CreateSolidBrush(colors.headerBackground);
			headerHotBackground = ::CreateSolidBrush(colors.headerHotBackground);

			::DeleteObject(edge);

			edge = ::CreatePen(PS_SOLID, 1, colors.headerEdge);
		}
	};

	struct ThemeView
	{
		ColorsView _clrView;
		BrushesAndPensView _hbrPnView;

		ThemeView()
			: _clrView(darkColorsView)
			, _hbrPnView(_clrView)
		{}

		ThemeView(const ColorsView& colorsView)
			: _clrView(colorsView)
			, _hbrPnView(_clrView)
		{}

		void updateBrushesAndPens()
		{
			_hbrPnView.change(_clrView);
		}

		void change(ColorsView colors)
		{
			_clrView = colors;
			updateBrushesAndPens();
		}
	};

	static ThemeView tView{darkColorsView};

	static ThemeView& getThemeView()
	{
		return tView;
	}

	static bool g_useDarkMode = true;
	static bool g_enableWindowsMode = false;

	static auto g_mica = DWMSBT_AUTO;
	static bool g_micaExtend = false;

	void initOptions()
	{
		std::wstring iniPath = getIniPath(L"7zDark");
		if (fileExists(iniPath))
		{
			switch (::GetPrivateProfileInt(L"main", L"mode", 1, iniPath.c_str()))
			{
				case 0:
				{
					g_useDarkMode = false;
					g_enableWindowsMode = false;
					break;
				}

				case 2:
				{
					g_useDarkMode = DarkMode::isDarkModeReg();
					g_enableWindowsMode = true;
					break;
				}

				case 1:
				default:
				{
					g_useDarkMode = true;
					g_enableWindowsMode = false;
					break;
				}
			}

			std::wstring sectionBase = g_useDarkMode ? L"dark" : L"light";
			std::wstring sectionColorsView = sectionBase + L".colors.view";
			std::wstring sectionColors = sectionBase + L".colors";

			switch (::GetPrivateProfileInt(sectionBase.c_str(), L"mica", 0, iniPath.c_str()))
			{
				case 1:
				{
					g_mica = DWMSBT_NONE;
					break;
				}

				case 2:
				{
					g_mica = DWMSBT_MAINWINDOW;
					break;
				}

				case 3:
				{
					g_mica = DWMSBT_TRANSIENTWINDOW;
					break;
				}

				case 4:
				{
					g_mica = DWMSBT_TABBEDWINDOW;
					break;
				}

				default:
				{
					g_mica = DWMSBT_AUTO;
					break;
				}
			}

			if (g_useDarkMode)
			{
				int tone = ::GetPrivateProfileInt(sectionBase.c_str(), L"tone", 0, iniPath.c_str());
				if (tone > 6)
					tone = 0;

				DarkMode::setDarkCustomColors(static_cast<DarkMode::ColorTone>(tone));
				DarkMode::getTheme()._colors = DarkMode::darkCustomizedColors;
				DarkMode::darkColorsView.headerBackground = DarkMode::getTheme()._colors.background;
				DarkMode::darkColorsView.headerHotBackground = DarkMode::getTheme()._colors.hotBackground;
				DarkMode::darkColorsView.headerText = DarkMode::getTheme()._colors.darkerText;
				DarkMode::getThemeView()._clrView = DarkMode::darkColorsView;

				if (!g_enableWindowsMode)
					g_micaExtend = (::GetPrivateProfileInt(sectionBase.c_str(), L"micaExtend", 0, iniPath.c_str()) == 1);
			}
			else
			{
				DarkMode::getTheme()._colors = DarkMode::lightColors;
				DarkMode::getThemeView()._clrView = DarkMode::lightColorsView;
			}

			setClrFromIni(sectionColorsView, L"backgroundView", iniPath, &DarkMode::getThemeView()._clrView.background);
			setClrFromIni(sectionColorsView, L"textView", iniPath, &DarkMode::getThemeView()._clrView.text);
			setClrFromIni(sectionColorsView, L"gridlines", iniPath, &DarkMode::getThemeView()._clrView.gridlines);
			setClrFromIni(sectionColorsView, L"backgroundHeader", iniPath, &DarkMode::getThemeView()._clrView.headerBackground);
			setClrFromIni(sectionColorsView, L"backgroundHotHeader", iniPath, &DarkMode::getThemeView()._clrView.headerHotBackground);
			setClrFromIni(sectionColorsView, L"textHeader", iniPath, &DarkMode::getThemeView()._clrView.headerText);
			setClrFromIni(sectionColorsView, L"edgeHeader", iniPath, &DarkMode::getThemeView()._clrView.headerEdge);

			setClrFromIni(sectionColors, L"background", iniPath, &DarkMode::getTheme()._colors.background);
			setClrFromIni(sectionColors, L"backgroundInteractive", iniPath, &DarkMode::getTheme()._colors.controlBackground);
			setClrFromIni(sectionColors, L"backgroundHot", iniPath, &DarkMode::getTheme()._colors.hotBackground);
			setClrFromIni(sectionColors, L"backgroundDlg", iniPath, &DarkMode::getTheme()._colors.dlgBackground);
			//setClrFromIni(sectionColors, L"backgroundError", iniPath, getTheme()._colors.errorBackground);

			setClrFromIni(sectionColors, L"text", iniPath, &DarkMode::getTheme()._colors.text);
			setClrFromIni(sectionColors, L"textItem", iniPath, &DarkMode::getTheme()._colors.darkerText);
			setClrFromIni(sectionColors, L"textDisabled", iniPath, &DarkMode::getTheme()._colors.disabledText);
			//setClrFromIni(sectionColors, L"textLink", iniPath, getTheme()._colors.linkText);

			setClrFromIni(sectionColors, L"edge", iniPath, &DarkMode::getTheme()._colors.edge);
			setClrFromIni(sectionColors, L"edgeHot", iniPath, &DarkMode::getTheme()._colors.hotEdge);
			setClrFromIni(sectionColors, L"edgeDisabled", iniPath, &DarkMode::getTheme()._colors.disabledEdge);

			DarkMode::getTheme()._brushes.change(DarkMode::getTheme()._colors);
			DarkMode::getTheme()._pens.change(DarkMode::getTheme()._colors);

			DarkMode::getThemeView().updateBrushesAndPens();
		}
	}

	static bool g_isInit = false;
	static bool g_isInitExperimental = false;

	void initDarkMode()
	{
		if (!g_isInit)
		{
			if (!g_isInitExperimental)
			{
				DarkMode::initExperimentalDarkMode();
				g_isInitExperimental = true;
			}

			initOptions();

			DarkMode::calculateTreeViewStyle();
			DarkMode::setDarkMode(g_useDarkMode, true);

			DarkMode::setSysColor(COLOR_WINDOW, DarkMode::getBackgroundColor());
			DarkMode::setSysColor(COLOR_WINDOWTEXT, DarkMode::getTextColor());
			DarkMode::setSysColor(COLOR_BTNFACE, DarkMode::getViewGridlinesColor());

			g_isInit = true;
		}
	}

	bool isEnabled()
	{
		return DarkMode::isWindows10();
	}

	bool isExperimentalActive()
	{
		return g_darkModeEnabled;
	}

	bool isExperimentalSupported()
	{
		return g_darkModeSupported;
	}

	bool isWindowsModeEnabled()
{
	return g_enableWindowsMode;
}

	bool isWindows10()
	{
		return IsWindows10();
	}

	bool isWindows11()
	{
		return IsWindows11();
	}

	DWORD getWindowsBuildNumber()
	{
		return GetWindowsBuildNumber();
	}

	static TreeViewStyle g_treeViewStylePrev = TreeViewStyle::classic;
	static TreeViewStyle g_treeViewStyle = TreeViewStyle::classic;
	static COLORREF g_treeViewBg = RGB(41, 49, 52);
	static double g_lightnessTreeView = 50.0;

	// adapted from https://stackoverflow.com/a/56678483
	double calculatePerceivedLightness(COLORREF c)
	{
		auto linearValue = [](double colorChannel) -> double {
			colorChannel /= 255.0;
			if (colorChannel <= 0.04045)
				return colorChannel / 12.92;
			return std::pow(((colorChannel + 0.055) / 1.055), 2.4);
			};

		double r = linearValue(static_cast<double>(GetRValue(c)));
		double g = linearValue(static_cast<double>(GetGValue(c)));
		double b = linearValue(static_cast<double>(GetBValue(c)));

		double luminance = 0.2126 * r + 0.7152 * g + 0.0722 * b;

		double lightness = (luminance <= 216.0 / 24389.0) ? (luminance * 24389.0 / 27.0) : (std::pow(luminance, (1.0 / 3.0)) * 116.0 - 16.0);
		return lightness;
	}

	COLORREF getBackgroundColor()         { return getTheme()._colors.background; }
	COLORREF getControlBackgroundColor()  { return getTheme()._colors.controlBackground; }
	COLORREF getHotBackgroundColor()      { return getTheme()._colors.hotBackground; }
	COLORREF getDlgBackgroundColor()      { return getTheme()._colors.dlgBackground; }
	COLORREF getErrorBackgroundColor()    { return getTheme()._colors.errorBackground; }
	COLORREF getTextColor()               { return getTheme()._colors.text; }
	COLORREF getDarkerTextColor()         { return getTheme()._colors.darkerText; }
	COLORREF getDisabledTextColor()       { return getTheme()._colors.disabledText; }
	COLORREF getLinkTextColor()           { return getTheme()._colors.linkText; }
	COLORREF getEdgeColor()               { return getTheme()._colors.edge; }
	COLORREF getHotEdgeColor()            { return getTheme()._colors.hotEdge; }
	COLORREF getDisabledEdgeColor()       { return getTheme()._colors.disabledEdge; }

	HBRUSH getBackgroundBrush()           { return getTheme()._brushes.background; }
	HBRUSH getControlBackgroundBrush()    { return getTheme()._brushes.controlBackground; }
	HBRUSH getHotBackgroundBrush()        { return getTheme()._brushes.hotBackground; }
	HBRUSH getDlgBackgroundBrush()        { return getTheme()._brushes.dlgBackground; }
	HBRUSH getErrorBackgroundBrush()      { return getTheme()._brushes.errorBackground; }

	HBRUSH getEdgeBrush()                 { return getTheme()._brushes.edge; }
	HBRUSH getHotEdgeBrush()              { return getTheme()._brushes.hotEdge; }
	HBRUSH getDisabledEdgeBrush()         { return getTheme()._brushes.disabledEdge; }

	HPEN getDarkerTextPen()               { return getTheme()._pens.darkerTextPen; }
	HPEN getEdgePen()                     { return getTheme()._pens.edgePen; }
	HPEN getHotEdgePen()                  { return getTheme()._pens.hotEdgePen; }
	HPEN getDisabledEdgePen()             { return getTheme()._pens.disabledEdgePen; }

	void changeCustomTheme(const Colors& colors)
	{
		tDefault.change(colors);
	}

	COLORREF getViewBackgroundColor()       { return DarkMode::getThemeView()._clrView.background; }
	COLORREF getViewTextColor()             { return DarkMode::getThemeView()._clrView.text; }
	COLORREF getViewGridlinesColor()        { return DarkMode::getThemeView()._clrView.gridlines; }
	COLORREF getHeaderTextColor()           { return DarkMode::getThemeView()._clrView.headerText; }
	HBRUSH getViewBackgroundBrush()         { return DarkMode::getThemeView()._hbrPnView.background; }
	HBRUSH getViewGridlinesBrush()          { return DarkMode::getThemeView()._hbrPnView.gridlines; }
	HBRUSH getHeaderBackgroundBrush()       { return DarkMode::getThemeView()._hbrPnView.headerBackground; }
	HBRUSH getHeaderHotBackgroundBrush()    { return DarkMode::getThemeView()._hbrPnView.headerHotBackground; }
	HPEN getHeaderEdgePen()                 { return DarkMode::getThemeView()._hbrPnView.edge; }

	bool handleSettingChange(LPARAM lParam)
	{
		if (DarkMode::isExperimentalSupported() && IsColorSchemeChangeMessage(lParam))
		{
			// ShouldAppsUseDarkMode() is not reliable from 1903+, use NppDarkMode::isDarkModeReg() instead
			bool isDarkModeUsed = DarkMode::isDarkModeReg() && !IsHighContrast();
			if (DarkMode::isExperimentalActive() != isDarkModeUsed)
			{
				g_darkModeEnabled = isDarkModeUsed;
				if (g_isInit)
				{
					g_isInit = false;
					DarkMode::initDarkMode();
				}
			}
			return true;
		}
		return false;
	}

	bool isDarkModeReg()
	{
		DWORD data{};
		DWORD dwBufSize = sizeof(data);
		LPCWSTR lpSubKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
		LPCWSTR lpValue = L"AppsUseLightTheme";

		auto result = RegGetValueW(HKEY_CURRENT_USER, lpSubKey, lpValue, RRF_RT_REG_DWORD, nullptr, &data, &dwBufSize);
		if (result != ERROR_SUCCESS)
		{
			return false;
		}

		// dark mode is 0, light mode is 1
		return data == 0UL;
	}

	// processes messages related to UAH / custom menubar drawing.
	// return true if handled, false to continue with normal processing in your wndproc
	bool runUAHWndProc(HWND hWnd, UINT uMsg, WPARAM /*wParam*/, LPARAM lParam, LRESULT* lr)
	{
		static HTHEME g_menuTheme = nullptr;

		switch (uMsg)
		{
			case WM_UAHDRAWMENU:
			{
				auto pUDM = reinterpret_cast<UAHMENU*>(lParam);
				RECT rc{};

				// get the menubar rect
				{
					MENUBARINFO mbi{};
					mbi.cbSize = sizeof(MENUBARINFO);
					::GetMenuBarInfo(hWnd, OBJID_MENU, 0, &mbi);

					RECT rcWindow{};
					::GetWindowRect(hWnd, &rcWindow);

					// the rcBar is offset by the window rect
					rc = mbi.rcBar;
					::OffsetRect(&rc, -rcWindow.left, -rcWindow.top);

					rc.top -= 1;
				}

				::FillRect(pUDM->hdc, &rc, DarkMode::getDlgBackgroundBrush());

				*lr = 0;

				return true;
			}

			case WM_UAHDRAWMENUITEM:
			{
				auto pUDMI = reinterpret_cast<UAHDRAWMENUITEM*>(lParam);

				// get the menu item string
				wchar_t menuString[256] = { '\0' };
				MENUITEMINFO mii{};
				{
					mii.cbSize = sizeof(MENUITEMINFO);
					mii.fMask = MIIM_STRING;
					mii.dwTypeData = menuString;
					mii.cch = (sizeof(menuString) / 2) - 1;

					GetMenuItemInfo(pUDMI->um.hmenu, pUDMI->umi.iPosition, TRUE, &mii);
				}

				// get the item state for drawing

				DWORD dwFlags = DT_CENTER | DT_SINGLELINE | DT_VCENTER;

				int iTextStateID = MBI_NORMAL;
				int iBackgroundStateID = MBI_NORMAL;
				{
					if ((pUDMI->dis.itemState & ODS_INACTIVE) | (pUDMI->dis.itemState & ODS_DEFAULT))
					{
						// normal display
						iTextStateID = MBI_NORMAL;
						iBackgroundStateID = MBI_NORMAL;
					}
					if (pUDMI->dis.itemState & ODS_HOTLIGHT)
					{
						// hot tracking
						iTextStateID = MBI_HOT;
						iBackgroundStateID = MBI_HOT;
					}
					if (pUDMI->dis.itemState & ODS_SELECTED)
					{
						// clicked
						iTextStateID = MBI_PUSHED;
						iBackgroundStateID = MBI_PUSHED;
					}
					if ((pUDMI->dis.itemState & ODS_GRAYED) || (pUDMI->dis.itemState & ODS_DISABLED))
					{
						// disabled / grey text
						iTextStateID = MBI_DISABLED;
						iBackgroundStateID = MBI_DISABLED;
					}
					if (pUDMI->dis.itemState & ODS_NOACCEL)
					{
						dwFlags |= DT_HIDEPREFIX;
					}
				}

				if (!g_menuTheme)
				{
					g_menuTheme = ::OpenThemeData(hWnd, VSCLASS_MENU);
				}

				switch (iBackgroundStateID)
				{
					case MBI_NORMAL:
					case MBI_DISABLED:
					{
						::FillRect(pUDMI->um.hdc, &pUDMI->dis.rcItem, DarkMode::getDlgBackgroundBrush());
						break;
					}

					case MBI_HOT:
					case MBI_DISABLEDHOT:
					{
						::FillRect(pUDMI->um.hdc, &pUDMI->dis.rcItem, DarkMode::getHotBackgroundBrush());
						break;
					}

					case MBI_PUSHED:
					case MBI_DISABLEDPUSHED:
					{
						::FillRect(pUDMI->um.hdc, &pUDMI->dis.rcItem, DarkMode::getControlBackgroundBrush());
						break;
					}

					default:
					{
						::DrawThemeBackground(g_menuTheme, pUDMI->um.hdc, MENU_BARITEM, iBackgroundStateID, &pUDMI->dis.rcItem, nullptr);
						break;
					}
				}

				DTTOPTS dttopts{};
				dttopts.dwSize = sizeof(DTTOPTS);
				if (iTextStateID == MBI_NORMAL || iTextStateID == MBI_HOT || iTextStateID == MBI_PUSHED)
				{
					dttopts.dwFlags |= DTT_TEXTCOLOR;
					dttopts.crText = DarkMode::getTextColor();
				}
				else if (iTextStateID == MBI_DISABLED || iTextStateID == MBI_DISABLEDHOT || iTextStateID == MBI_DISABLEDPUSHED)
				{
					dttopts.dwFlags |= DTT_TEXTCOLOR;
					dttopts.crText = DarkMode::getDisabledTextColor();
				}

				::DrawThemeTextEx(g_menuTheme, pUDMI->um.hdc, MENU_BARITEM, iTextStateID, menuString, mii.cch, dwFlags, &pUDMI->dis.rcItem, &dttopts);

				*lr = 0;

				return true;
			}

			case WM_NCDESTROY:
			case WM_DPICHANGED:
			case WM_THEMECHANGED:
			{
				if (g_menuTheme)
				{
					::CloseThemeData(g_menuTheme);
					g_menuTheme = nullptr;
				}
				// continue processing in main wndproc
				return false;
			}

			default:
				return false;
		}
	}

	void drawUAHMenuNCBottomLine(HWND hWnd)
	{
		MENUBARINFO mbi{};
		mbi.cbSize = sizeof(MENUBARINFO);
		if (!::GetMenuBarInfo(hWnd, OBJID_MENU, 0, &mbi))
		{
			return;
		}

		RECT rcClient{};
		::GetClientRect(hWnd, &rcClient);
		::MapWindowPoints(hWnd, nullptr, reinterpret_cast<POINT*>(&rcClient), 2);

		RECT rcWindow{};
		::GetWindowRect(hWnd, &rcWindow);

		::OffsetRect(&rcClient, -rcWindow.left, -rcWindow.top);

		// the rcBar is offset by the window rect
		RECT rcAnnoyingLine = rcClient;
		rcAnnoyingLine.bottom = rcAnnoyingLine.top;
		rcAnnoyingLine.top--;


		HDC hdc = ::GetWindowDC(hWnd);
		::FillRect(hdc, &rcAnnoyingLine, DarkMode::getDlgBackgroundBrush());
		::ReleaseDC(hWnd, hdc);
	}

	// from DarkMode.h

	void initExperimentalDarkMode()
	{
		::InitDarkMode();
	}

	void setDarkMode(bool useDark, bool fixDarkScrollbar)
	{
		::SetDarkMode(useDark, fixDarkScrollbar);
	}

	void allowDarkModeForApp(bool allow)
	{
		::AllowDarkModeForApp(allow);
	}

	bool allowDarkModeForWindow(HWND hWnd, bool allow)
	{
		return ::AllowDarkModeForWindow(hWnd, allow);
	}

	void setTitleBarThemeColor(HWND hWnd)
	{
		::RefreshTitleBarThemeColor(hWnd);
	}

	void setSysColor(int nIndex, COLORREF color)
	{
		::SetMySysColor(nIndex, color);
	}

	bool hookSysColor()
	{
		return ::HookSysColor();
	}
	void unhookSysColor()
	{
		::UnhookSysColor();
	}

	void enableDarkScrollBarForWindowAndChildren(HWND hWnd)
	{
		::EnableDarkScrollBarForWindowAndChildren(hWnd);
	}

	void paintRoundFrameRect(HDC hdc, const RECT rect, const HPEN hpen, int width, int height)
	{
		auto holdBrush = ::SelectObject(hdc, ::GetStockObject(NULL_BRUSH));
		auto holdPen = ::SelectObject(hdc, hpen);
		::RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, width, height);
		::SelectObject(hdc, holdBrush);
		::SelectObject(hdc, holdPen);
	}

	struct BaseControlThemeData
	{
		HTHEME _hTheme = nullptr;
	
		BaseControlThemeData() = default;
	
		virtual ~BaseControlThemeData()
		{
			closeTheme();
		}
	
		virtual const wchar_t* getThemeClass() const = 0;
	
		bool ensureTheme(HWND hWnd)
		{
			if (!_hTheme)
			{
				_hTheme = ::OpenThemeData(hWnd, getThemeClass());
			}
			return _hTheme != nullptr;
		}
	
		void closeTheme()
		{
			if (_hTheme)
			{
				::CloseThemeData(_hTheme);
				_hTheme = nullptr;
			}
		}
	};

	struct BaseControlBufferData : public BaseControlThemeData
	{
		HDC _hMemDC = nullptr;
		HBITMAP _hMemBmp = nullptr;
		HBITMAP _hOldBmp = nullptr;
		SIZE _szBuffer{};

		BaseControlBufferData() = default;

		~BaseControlBufferData() override
		{
			releaseBuffer();
		}

		bool ensureBuffer(HDC hdc, const RECT& rcClient)
		{
			int width = rcClient.right - rcClient.left;
			int height = rcClient.bottom - rcClient.top;

			if (_szBuffer.cx != width || _szBuffer.cy != height)
			{
				releaseBuffer();
				_hMemDC = ::CreateCompatibleDC(hdc);
				_hMemBmp = ::CreateCompatibleBitmap(hdc, width, height);
				_hOldBmp = static_cast<HBITMAP>(::SelectObject(_hMemDC, _hMemBmp));
				_szBuffer = { width, height };
			}

			return _hMemDC != nullptr && _hMemBmp != nullptr;
		}

		void releaseBuffer()
		{
			if (_hMemDC)
			{
				::SelectObject(_hMemDC, _hOldBmp);
				::DeleteObject(_hMemBmp);
				::DeleteDC(_hMemDC);

				_hMemDC = nullptr;
				_hMemBmp = nullptr;
				_hOldBmp = nullptr;
				_szBuffer = { 0, 0 };
			}
		}
	};

	struct ButtonData : public BaseControlThemeData
	{
		int _iStateID = 0;
		bool _isSizeSet = false;
		SIZE _szBtn{};

		ButtonData() = default;

		// Saves width and height from the resource file for use as restrictions.
		ButtonData(HWND hWnd)
		{
			const auto nBtnStyle = ::GetWindowLongPtr(hWnd, GWL_STYLE);
			switch (nBtnStyle & BS_TYPEMASK)
			{
				case BS_CHECKBOX:
				case BS_AUTOCHECKBOX:
				case BS_RADIOBUTTON:
				case BS_AUTORADIOBUTTON:
				{
					if ((nBtnStyle & BS_MULTILINE) != BS_MULTILINE)
					{
						RECT rcBtn{};
						::GetClientRect(hWnd, &rcBtn);
						_szBtn.cx = rcBtn.right - rcBtn.left;
						_szBtn.cy = rcBtn.bottom - rcBtn.top;
						_isSizeSet = (_szBtn.cx != 0 && _szBtn.cy != 0);
					}
					break;
				}

				default:
					break;
			}
		}

		~ButtonData() override = default;

		const wchar_t* getThemeClass() const override
		{
			return VSCLASS_BUTTON;
		}
	};

	static void renderButton(HWND hWnd, HDC hdc, HTHEME hTheme, int iPartID, int iStateID)
	{
		RECT rcClient{};
		wchar_t szText[256] = { '\0' };
		const auto nState = static_cast<DWORD>(::SendMessage(hWnd, BM_GETSTATE, 0, 0));
		const auto uiState = static_cast<DWORD>(::SendMessage(hWnd, WM_QUERYUISTATE, 0, 0));
		const auto nStyle = ::GetWindowLongPtr(hWnd, GWL_STYLE);

		HFONT hFont = nullptr;
		HFONT hOldFont = nullptr;
		HFONT hCreatedFont = nullptr;
		LOGFONT lf{};
		if (SUCCEEDED(::GetThemeFont(hTheme, hdc, iPartID, iStateID, TMT_FONT, &lf)))
		{
			hCreatedFont = ::CreateFontIndirect(&lf);
			hFont = hCreatedFont;
		}

		if (!hFont)
		{
			hFont = reinterpret_cast<HFONT>(::SendMessage(hWnd, WM_GETFONT, 0, 0));
		}

		hOldFont = static_cast<HFONT>(::SelectObject(hdc, hFont));

		DWORD dtFlags = DT_LEFT; // DT_LEFT is 0
		dtFlags |= (nStyle & BS_MULTILINE) ? DT_WORDBREAK : DT_SINGLELINE;
		dtFlags |= ((nStyle & BS_CENTER) == BS_CENTER) ? DT_CENTER : (nStyle & BS_RIGHT) ? DT_RIGHT : 0;
		dtFlags |= ((nStyle & BS_VCENTER) == BS_VCENTER) ? DT_VCENTER : (nStyle & BS_BOTTOM) ? DT_BOTTOM : 0;
		dtFlags |= (uiState & UISF_HIDEACCEL) ? DT_HIDEPREFIX : 0;

		if (!(nStyle & BS_MULTILINE) && !(nStyle & BS_BOTTOM) && !(nStyle & BS_TOP))
		{
			dtFlags |= DT_VCENTER;
		}

		::GetClientRect(hWnd, &rcClient);
		::GetWindowText(hWnd, szText, _countof(szText));

		SIZE szBox{ 13, 13 };
		::GetThemePartSize(hTheme, hdc, iPartID, iStateID, nullptr, TS_DRAW, &szBox);

		RECT rcText = rcClient;
		::GetThemeBackgroundContentRect(hTheme, hdc, iPartID, iStateID, &rcClient, &rcText);

		RECT rcBackground = rcClient;
		if ((dtFlags & DT_SINGLELINE) == DT_SINGLELINE)
		{
			rcBackground.top += (rcText.bottom - rcText.top - szBox.cy) / 2;
		}
		rcBackground.bottom = rcBackground.top + szBox.cy;
		rcBackground.right = rcBackground.left + szBox.cx;
		rcText.left = rcBackground.right + 3;

		::DrawThemeParentBackground(hWnd, hdc, &rcClient);
		::DrawThemeBackground(hTheme, hdc, iPartID, iStateID, &rcBackground, nullptr);

		DTTOPTS dtto{};
		dtto.dwSize = sizeof(DTTOPTS);
		dtto.dwFlags = DTT_TEXTCOLOR;
		dtto.crText = (::IsWindowEnabled(hWnd) == FALSE) ? DarkMode::getDisabledTextColor() : DarkMode::getTextColor();

		::DrawThemeTextEx(hTheme, hdc, iPartID, iStateID, szText, -1, dtFlags, &rcText, &dtto);

		if (((nState & BST_FOCUS) == BST_FOCUS) && ((uiState & UISF_HIDEFOCUS) != UISF_HIDEFOCUS))
		{
			RECT rcTextOut = rcText;
			dtto.dwFlags |= DTT_CALCRECT;
			::DrawThemeTextEx(hTheme, hdc, iPartID, iStateID, szText, -1, dtFlags | DT_CALCRECT, &rcTextOut, &dtto);
			RECT rcFocus = rcTextOut;
			rcFocus.bottom++;
			rcFocus.left--;
			rcFocus.right++;
			::DrawFocusRect(hdc, &rcFocus);
		}

		if (hCreatedFont) ::DeleteObject(hCreatedFont);
		::SelectObject(hdc, hOldFont);
	}

	static void paintButton(HWND hWnd, HDC hdc, ButtonData& buttonData)
	{
		const auto nState = static_cast<DWORD>(::SendMessage(hWnd, BM_GETSTATE, 0, 0));
		const auto nStyle = ::GetWindowLongPtr(hWnd, GWL_STYLE);
		const auto nButtonStyle = nStyle & BS_TYPEMASK;

		int iPartID = BP_CHECKBOX;

		// Plugin might use BS_3STATE and BS_AUTO3STATE button style
		if (nButtonStyle == BS_CHECKBOX || nButtonStyle == BS_AUTOCHECKBOX || nButtonStyle == BS_3STATE || nButtonStyle == BS_AUTO3STATE)
		{
			iPartID = BP_CHECKBOX;
		}
		else if (nButtonStyle == BS_RADIOBUTTON || nButtonStyle == BS_AUTORADIOBUTTON)
		{
			iPartID = BP_RADIOBUTTON;
		}
		else
		{
			//assert(false);
		}

		// states of BP_CHECKBOX and BP_RADIOBUTTON are the same
		int iStateID = RBS_UNCHECKEDNORMAL;

		if (::IsWindowEnabled(hWnd) == FALSE)           iStateID = RBS_UNCHECKEDDISABLED;
		else if ((nState & BST_PUSHED) == BST_PUSHED)   iStateID = RBS_UNCHECKEDPRESSED;
		else if ((nState & BST_HOT) == BST_HOT)         iStateID = RBS_UNCHECKEDHOT;

		if ((nState & BST_CHECKED) == BST_CHECKED)      iStateID += 4;

		if (::BufferedPaintRenderAnimation(hWnd, hdc))
		{
			return;
		}

		BP_ANIMATIONPARAMS animParams{};
		animParams.cbSize = sizeof(BP_ANIMATIONPARAMS);
		animParams.style = BPAS_LINEAR;
		if (iStateID != buttonData._iStateID)
		{
			::GetThemeTransitionDuration(buttonData._hTheme, iPartID, buttonData._iStateID, iStateID, TMT_TRANSITIONDURATIONS, &animParams.dwDuration);
		}

		RECT rcClient{};
		::GetClientRect(hWnd, &rcClient);

		HDC hdcFrom = nullptr;
		HDC hdcTo = nullptr;
		HANIMATIONBUFFER hbpAnimation = ::BeginBufferedAnimation(hWnd, hdc, &rcClient, BPBF_COMPATIBLEBITMAP, nullptr, &animParams, &hdcFrom, &hdcTo);
		if (hbpAnimation)
		{
			if (hdcFrom)
			{
				DarkMode::renderButton(hWnd, hdcFrom, buttonData._hTheme, iPartID, buttonData._iStateID);
			}
			if (hdcTo)
			{
				DarkMode::renderButton(hWnd, hdcTo, buttonData._hTheme, iPartID, iStateID);
			}

			buttonData._iStateID = iStateID;

			::EndBufferedAnimation(hbpAnimation, TRUE);
		}
		else
		{
			DarkMode::renderButton(hWnd, hdc, buttonData._hTheme, iPartID, iStateID);

			buttonData._iStateID = iStateID;
		}
	}

	constexpr UINT_PTR g_buttonSubclassID = 42;

	static LRESULT CALLBACK ButtonSubclass(
		HWND hWnd,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam,
		UINT_PTR uIdSubclass,
		DWORD_PTR dwRefData
	)
	{
		auto pButtonData = reinterpret_cast<ButtonData*>(dwRefData);

		switch (uMsg)
		{
			case WM_NCDESTROY:
			{
				::RemoveWindowSubclass(hWnd, ButtonSubclass, uIdSubclass);
				delete pButtonData;
				break;
			}

			case WM_ERASEBKGND:
			{
				if (DarkMode::isEnabled() && pButtonData->ensureTheme(hWnd))
				{
					return TRUE;
				}
				break;
			}

			case WM_PRINTCLIENT:
			case WM_PAINT:
			{
				if (DarkMode::isEnabled() && pButtonData->ensureTheme(hWnd))
				{
					PAINTSTRUCT ps{};
					HDC hdc = reinterpret_cast<HDC>(wParam);
					if (!hdc)
					{
						hdc = ::BeginPaint(hWnd, &ps);
					}

					DarkMode::paintButton(hWnd, hdc, *pButtonData);

					if (ps.hdc)
					{
						::EndPaint(hWnd, &ps);
					}

					return 0;
				}
				break;
			}

			case WM_DPICHANGED:
			{
				pButtonData->closeTheme();
				return 0;
			}

			case WM_THEMECHANGED:
			{
				pButtonData->closeTheme();
				break;
			}

			case WM_SIZE:
			case WM_DESTROY:
			{
				::BufferedPaintStopAllAnimations(hWnd);
				break;
			}

			case WM_ENABLE:
			{
				if (DarkMode::isEnabled())
				{
					// skip the button's normal wndproc so it won't redraw out of wm_paint
					LRESULT lr = ::DefWindowProc(hWnd, uMsg, wParam, lParam);
					::InvalidateRect(hWnd, nullptr, FALSE);
					return lr;
				}
				break;
			}

			case WM_UPDATEUISTATE:
			{
				if (HIWORD(wParam) & (UISF_HIDEACCEL | UISF_HIDEFOCUS))
				{
					::InvalidateRect(hWnd, nullptr, FALSE);
				}
				break;
			}
		}
		return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	void subclassButtonControl(HWND hWnd)
	{
		if (::GetWindowSubclass(hWnd, ButtonSubclass, g_buttonSubclassID, nullptr) == FALSE)
		{
			DWORD_PTR pButtonData = reinterpret_cast<DWORD_PTR>(new ButtonData(hWnd));
			::SetWindowSubclass(hWnd, ButtonSubclass, g_buttonSubclassID, pButtonData);
		}
	}

	static void paintGroupbox(HWND hWnd, HDC hdc, ButtonData& buttonData)
	{
		const bool isDisabled = ::IsWindowEnabled(hWnd) == FALSE;
		int iPartID = BP_GROUPBOX;
		int iStateID = isDisabled ? GBS_DISABLED : GBS_NORMAL;

		RECT rcClient{};
		::GetClientRect(hWnd, &rcClient);

		rcClient.bottom -= 1;

		RECT rcText = rcClient;
		RECT rcBackground = rcClient;

		HFONT hFont = nullptr;
		HFONT hOldFont = nullptr;
		HFONT hCreatedFont = nullptr;
		LOGFONT lf{};
		if (SUCCEEDED(::GetThemeFont(buttonData._hTheme, hdc, iPartID, iStateID, TMT_FONT, &lf)))
		{
			hCreatedFont = ::CreateFontIndirect(&lf);
			hFont = hCreatedFont;
		}

		if (!hFont)
		{
			hFont = reinterpret_cast<HFONT>(::SendMessage(hWnd, WM_GETFONT, 0, 0));
		}

		hOldFont = static_cast<HFONT>(::SelectObject(hdc, hFont));

		wchar_t szText[256] = { '\0' };
		::GetWindowText(hWnd, szText, _countof(szText));

		const auto nStyle = ::GetWindowLongPtr(hWnd, GWL_STYLE);
		const bool isCenter = (nStyle & BS_CENTER) == BS_CENTER;

		if (szText[0])
		{
			SIZE textSize{};
			::GetTextExtentPoint32(hdc, szText, static_cast<int>(wcslen(szText)), &textSize);

			int centerPosX = isCenter ? ((rcClient.right - rcClient.left - textSize.cx) / 2) : 7;

			rcBackground.top += textSize.cy / 2;
			rcText.left += centerPosX;
			rcText.bottom = rcText.top + textSize.cy;
			rcText.right = rcText.left + textSize.cx + 4;

			::ExcludeClipRect(hdc, rcText.left, rcText.top, rcText.right, rcText.bottom);
		}
		else
		{
			SIZE textSize{};
			::GetTextExtentPoint32(hdc, L"M", 1, &textSize);
			rcBackground.top += textSize.cy / 2;
		}

		RECT rcContent = rcBackground;
		::GetThemeBackgroundContentRect(buttonData._hTheme, hdc, BP_GROUPBOX, iStateID, &rcBackground, &rcContent);
		::ExcludeClipRect(hdc, rcContent.left, rcContent.top, rcContent.right, rcContent.bottom);

		//DrawThemeParentBackground(hWnd, hdc, &rcClient);
		//DrawThemeBackground(buttonData._hTheme, hdc, BP_GROUPBOX, iStateID, &rcBackground, nullptr);
		DarkMode::paintRoundFrameRect(hdc, rcBackground, DarkMode::getEdgePen());

		::SelectClipRgn(hdc, nullptr);

		if (szText[0])
		{
			rcText.right -= 2;
			rcText.left += 2;

			DTTOPTS dtto{};
			dtto.dwSize = sizeof(DTTOPTS);
			dtto.dwFlags = DTT_TEXTCOLOR;
			dtto.crText = isDisabled ? DarkMode::getDisabledTextColor() : DarkMode::getTextColor();

			DWORD textFlags = isCenter ? DT_CENTER : DT_LEFT;

			if(::SendMessage(hWnd, WM_QUERYUISTATE, 0, 0) != static_cast<LRESULT>(NULL))
			{
				textFlags |= DT_HIDEPREFIX;
			}

			::DrawThemeTextEx(buttonData._hTheme, hdc, BP_GROUPBOX, iStateID, szText, -1, textFlags | DT_SINGLELINE, &rcText, &dtto);
		}

		if (hCreatedFont) ::DeleteObject(hCreatedFont);
		::SelectObject(hdc, hOldFont);
	}

	constexpr UINT_PTR g_groupboxSubclassID = 42;

	static LRESULT CALLBACK GroupboxSubclass(
		HWND hWnd,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam,
		UINT_PTR uIdSubclass,
		DWORD_PTR dwRefData
	)
	{
		auto pButtonData = reinterpret_cast<ButtonData*>(dwRefData);

		switch (uMsg)
		{
			case WM_NCDESTROY:
			{
				::RemoveWindowSubclass(hWnd, GroupboxSubclass, uIdSubclass);
				delete pButtonData;
				break;
			}

			case WM_ERASEBKGND:
			{
				if (DarkMode::isEnabled() && pButtonData->ensureTheme(hWnd))
				{
					return TRUE;
				}
				break;
			}

			case WM_PRINTCLIENT:
			case WM_PAINT:
			{
				if (DarkMode::isEnabled() && pButtonData->ensureTheme(hWnd))
				{
					PAINTSTRUCT ps{};
					HDC hdc = reinterpret_cast<HDC>(wParam);
					if (!hdc)
					{
						hdc = ::BeginPaint(hWnd, &ps);
					}

					DarkMode::paintGroupbox(hWnd, hdc, *pButtonData);

					if (ps.hdc)
					{
						::EndPaint(hWnd, &ps);
					}

					return 0;
				}
				break;
			}

			case WM_DPICHANGED:
			{
				pButtonData->closeTheme();
				return 0;
			}

			case WM_THEMECHANGED:
			{
				pButtonData->closeTheme();
				break;
			}

			case WM_ENABLE:
			{
				::RedrawWindow(hWnd, nullptr, nullptr, RDW_INVALIDATE);
				break;
			}
		}
		return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	void subclassGroupboxControl(HWND hWnd)
	{
		if (::GetWindowSubclass(hWnd, GroupboxSubclass, g_groupboxSubclassID, nullptr) == FALSE)
		{
			DWORD_PTR pButtonData = reinterpret_cast<DWORD_PTR>(new ButtonData());
			::SetWindowSubclass(hWnd, GroupboxSubclass, g_groupboxSubclassID, pButtonData);
		}
	}

	static void paintTab(HWND hWnd, HDC hdc, RECT& rc)
	{
		::FillRect(hdc, &rc, DarkMode::getDlgBackgroundBrush());

		auto holdPen = static_cast<HPEN>(::SelectObject(hdc, DarkMode::getEdgePen()));

		auto holdClip = ::CreateRectRgn(0, 0, 0, 0);
		if (1 != GetClipRgn(hdc, holdClip))
		{
			::DeleteObject(holdClip);
			holdClip = nullptr;
		}

		auto hFont = reinterpret_cast<HFONT>(::SendMessage(hWnd, WM_GETFONT, 0, 0));
		auto hOldFont = ::SelectObject(hdc, hFont);

		POINT ptCursor{};
		::GetCursorPos(&ptCursor);
		::ScreenToClient(hWnd, &ptCursor);

		int nTabs = TabCtrl_GetItemCount(hWnd);

		int nSelTab = TabCtrl_GetCurSel(hWnd);
		for (int i = 0; i < nTabs; ++i)
		{
			RECT rcItem{};
			TabCtrl_GetItemRect(hWnd, i, &rcItem);
			RECT rcFrame{ rcItem };

			RECT rcIntersect{};
			if (::IntersectRect(&rcIntersect, &rc, &rcItem))
			{
				const bool bHot = ::PtInRect(&rcItem, ptCursor);
				const bool isSelectedTab = (i == nSelTab);

				HRGN hClip = ::CreateRectRgnIndirect(&rcItem);

				::SelectClipRgn(hdc, hClip);

				::SetTextColor(hdc, (bHot || isSelectedTab ) ? DarkMode::getTextColor() : DarkMode::getDarkerTextColor());

				::InflateRect(&rcItem, -1, -1);
				rcItem.right += 1;

				// for consistency getBackgroundBrush() 
				// would be better, than getControlBackgroundBrush(),
				// however default getBackgroundBrush() has same color
				// as getDlgBackgroundBrush()
				::FillRect(hdc, &rcItem, isSelectedTab ? DarkMode::getDlgBackgroundBrush() : bHot ? DarkMode::getHotBackgroundBrush() : DarkMode::getControlBackgroundBrush());

				::SetBkMode(hdc, TRANSPARENT);

				wchar_t label[MAX_PATH]{};
				TCITEM tci{};
				tci.mask = TCIF_TEXT;
				tci.pszText = label;
				tci.cchTextMax = MAX_PATH - 1;

				::SendMessage(hWnd, TCM_GETITEM, i, reinterpret_cast<LPARAM>(&tci));

				RECT rcText{ rcItem };
				if (isSelectedTab)
				{
					::OffsetRect(&rcText, 0, -1);
					::InflateRect(&rcFrame, 0, 1);
				}

				if (i != nTabs - 1)
				{
					rcFrame.right += 1;
				}

				::FrameRect(hdc, &rcFrame, DarkMode::getEdgeBrush());

				::DrawText(hdc, label, -1, &rcText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

				::DeleteObject(hClip);

				::SelectClipRgn(hdc, holdClip);
			}
		}

		::SelectObject(hdc, hOldFont);

		::SelectClipRgn(hdc, holdClip);
		if (holdClip)
		{
			::DeleteObject(holdClip);
			holdClip = nullptr;
		}

		::SelectObject(hdc, holdPen);
	}

	constexpr UINT_PTR g_tabSubclassID = 42;

	static LRESULT CALLBACK TabSubclass(
		HWND hWnd,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam,
		UINT_PTR uIdSubclass,
		DWORD_PTR /*dwRefData*/
	)
	{
		switch (uMsg)
		{
			case WM_NCDESTROY:
			{
				::RemoveWindowSubclass(hWnd, TabSubclass, uIdSubclass);
				break;
			}

			case WM_ERASEBKGND:
			{
				if (DarkMode::isEnabled())
				{
					return TRUE;
				}
				break;
			}

			case WM_PAINT:
			{
				if (!DarkMode::isEnabled())
				{
					break;
				}

				const auto nStyle = ::GetWindowLongPtr(hWnd, GWL_STYLE);
				if ((nStyle & TCS_BUTTONS) || (nStyle & TCS_VERTICAL))
				{
					break;
				}

				PAINTSTRUCT ps{};
				auto hdc = ::BeginPaint(hWnd, &ps);
				
				DarkMode::paintTab(hWnd, hdc, ps.rcPaint);

				::EndPaint(hWnd, &ps);
				return 0;
			}

			case WM_PARENTNOTIFY:
			{
				switch (LOWORD(wParam))
				{
					case WM_CREATE:
					{
						auto hwndUpdown = reinterpret_cast<HWND>(lParam);
						if (DarkMode::subclassTabUpDownControl(hwndUpdown))
						{
							return 0;
						}
						break;
					}
				}
				return 0;
			}
		}
		return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	void subclassTabControl(HWND hWnd)
	{
		if (::GetWindowSubclass(hWnd, TabSubclass, g_tabSubclassID, nullptr) == FALSE)
		{
			::SetWindowSubclass(hWnd, TabSubclass, g_tabSubclassID, 0);
		}
	}

	void subclassTabControl(HWND hWnd, DarkModeParams p)
	{
		if (p._subclass)
		{
			subclassTabControl(hWnd);
		}
	}

	struct BorderMetricsData
	{
		UINT _dpi = USER_DEFAULT_SCREEN_DPI;
		LONG _xEdge = ::GetSystemMetrics(SM_CXEDGE);
		LONG _yEdge = ::GetSystemMetrics(SM_CYEDGE);
		LONG _xScroll = ::GetSystemMetrics(SM_CXVSCROLL);
		LONG _yScroll = ::GetSystemMetrics(SM_CYVSCROLL);
		bool _isHot = false;
	};

	constexpr UINT_PTR g_customBorderSubclassID = 42;

	static LRESULT CALLBACK CustomBorderSubclass(
		HWND hWnd,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam,
		UINT_PTR uIdSubclass,
		DWORD_PTR dwRefData
	)
	{
		auto pBorderMetricsData = reinterpret_cast<BorderMetricsData*>(dwRefData);

		switch (uMsg)
		{
			case WM_NCDESTROY:
			{
				::RemoveWindowSubclass(hWnd, CustomBorderSubclass, uIdSubclass);
				delete pBorderMetricsData;
				break;
			}

			case WM_NCPAINT:
			{
				if (!DarkMode::isEnabled())
				{
					break;
				}

				::DefSubclassProc(hWnd, uMsg, wParam, lParam);

				HDC hdc = ::GetWindowDC(hWnd);
				RECT rcClient{};
				::GetClientRect(hWnd, &rcClient);
				rcClient.right += (2 * pBorderMetricsData->_xEdge);

				const auto nStyle = ::GetWindowLongPtr(hWnd, GWL_STYLE);
				const bool hasVerScrollbar = (nStyle & WS_VSCROLL) == WS_VSCROLL;
				if (hasVerScrollbar)
				{
					rcClient.right += pBorderMetricsData->_xScroll;
				}

				rcClient.bottom += (2 * pBorderMetricsData->_yEdge);

				const bool hasHorScrollbar = (nStyle & WS_HSCROLL) == WS_HSCROLL;
				if (hasHorScrollbar)
				{
					rcClient.bottom += pBorderMetricsData->_yScroll;
				}

				HPEN hPen = ::CreatePen(PS_SOLID, 1, (::IsWindowEnabled(hWnd) == TRUE) ? DarkMode::getBackgroundColor() : DarkMode::getDlgBackgroundColor());
				RECT rcInner = rcClient;
				::InflateRect(&rcInner, -1, -1);
				DarkMode::paintRoundFrameRect(hdc, rcInner, hPen);
				::DeleteObject(hPen);

				POINT ptCursor{};
				::GetCursorPos(&ptCursor);
				::ScreenToClient(hWnd, &ptCursor);

				const bool isHot = ::PtInRect(&rcClient, ptCursor);
				const bool hasFocus = ::GetFocus() == hWnd;

				HPEN hEnabledPen = ((pBorderMetricsData->_isHot && isHot) || hasFocus ? DarkMode::getHotEdgePen() : DarkMode::getEdgePen());

				DarkMode::paintRoundFrameRect(hdc, rcClient, (::IsWindowEnabled(hWnd) == TRUE) ? hEnabledPen : DarkMode::getDisabledEdgePen());

				::ReleaseDC(hWnd, hdc);

				return 0;
			}

			case WM_NCCALCSIZE:
			{
				if (!DarkMode::isEnabled())
				{
					break;
				}

				auto lpRect = reinterpret_cast<LPRECT>(lParam);
				::InflateRect(lpRect, -(pBorderMetricsData->_xEdge), -(pBorderMetricsData->_yEdge));

				const auto nStyle = ::GetWindowLongPtr(hWnd, GWL_STYLE);
				const bool hasVerScrollbar = (nStyle & WS_VSCROLL) == WS_VSCROLL;
				if (hasVerScrollbar)
				{
					lpRect->right -= pBorderMetricsData->_xScroll;
				}

				const bool hasHorScrollbar = (nStyle & WS_HSCROLL) == WS_HSCROLL;
				if (hasHorScrollbar)
				{
					lpRect->bottom -= pBorderMetricsData->_yScroll;
				}

				return 0;
			}

			case WM_DPICHANGED:
			{
				::SetWindowPos(hWnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
				return 0;
			}

			case WM_MOUSEMOVE:
			{
				if (!DarkMode::isEnabled())
				{
					break;
				}

				if (::GetFocus() == hWnd)
				{
					break;
				}

				TRACKMOUSEEVENT tme{};
				tme.cbSize = sizeof(TRACKMOUSEEVENT);
				tme.dwFlags = TME_LEAVE;
				tme.hwndTrack = hWnd;
				tme.dwHoverTime = HOVER_DEFAULT;
				::TrackMouseEvent(&tme);

				if (!pBorderMetricsData->_isHot)
				{
					pBorderMetricsData->_isHot = true;
					::SetWindowPos(hWnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
				}
				break;
			}

			case WM_MOUSELEAVE:
			{
				if (!DarkMode::isEnabled())
				{
					break;
				}

				if (pBorderMetricsData->_isHot)
				{
					pBorderMetricsData->_isHot = false;
					::SetWindowPos(hWnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
				}

				TRACKMOUSEEVENT tme{};
				tme.cbSize = sizeof(TRACKMOUSEEVENT);
				tme.dwFlags = TME_LEAVE | TME_CANCEL;
				tme.hwndTrack = hWnd;
				tme.dwHoverTime = HOVER_DEFAULT;
				::TrackMouseEvent(&tme);
				break;
			}
		}
		return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	static void subclassCustomBorderForListBoxAndEditControls(HWND hWnd)
	{
		if (::GetWindowSubclass(hWnd, CustomBorderSubclass, g_customBorderSubclassID, nullptr) == FALSE)
		{
			auto pBorderMetricsData = reinterpret_cast<DWORD_PTR>(new BorderMetricsData());
			::SetWindowSubclass(hWnd, CustomBorderSubclass, g_customBorderSubclassID, pBorderMetricsData);
		}
	}

	struct ComboboxData : public BaseControlBufferData
	{
		LONG_PTR _cbStyle = CBS_SIMPLE;

		ComboboxData() = default;

		ComboboxData(LONG_PTR cbStyle)
			: _cbStyle(cbStyle) {}

		~ComboboxData() override = default;

		const wchar_t* getThemeClass() const override
		{
			return VSCLASS_COMBOBOX;
		}
	};

	void paintCombobox(HWND hWnd, HDC hdc, ComboboxData& comboboxData)
	{
		COMBOBOXINFO cbi{};
		cbi.cbSize = sizeof(COMBOBOXINFO);
		::GetComboBoxInfo(hWnd, &cbi);

		RECT rcClient{};
		::GetClientRect(hWnd, &rcClient);

		POINT ptCursor{};
		::GetCursorPos(&ptCursor);
		::ScreenToClient(hWnd, &ptCursor);

		const bool isDisabled = ::IsWindowEnabled(hWnd) == FALSE;
		const bool isHot = ::PtInRect(&rcClient, ptCursor) == TRUE && !isDisabled;
		
		bool hasFocus = false;

		::SelectObject(hdc, reinterpret_cast<HFONT>(::SendMessage(hWnd, WM_GETFONT, 0, 0)));
		//::SetBkMode(hdc, TRANSPARENT);

		RECT rcArrow{ cbi.rcButton };
		rcArrow.left -= 1;

		HBRUSH hSelectedBrush = isDisabled ? DarkMode::getDlgBackgroundBrush() : (isHot ? DarkMode::getHotBackgroundBrush() : DarkMode::getControlBackgroundBrush());

		// CBS_DROPDOWN text is handled by parent by WM_CTLCOLOREDIT
		if (comboboxData._cbStyle == CBS_DROPDOWNLIST)
		{
			// erase background on item change
			::FillRect(hdc, &rcClient, hSelectedBrush);

			auto index = static_cast<int>(::SendMessage(hWnd, CB_GETCURSEL, 0, 0));
			if (index != CB_ERR)
			{
				auto bufferLen = static_cast<size_t>(::SendMessage(hWnd, CB_GETLBTEXTLEN, index, 0));
				wchar_t* buffer = new wchar_t[(bufferLen + 1)];
				::SendMessage(hWnd, CB_GETLBTEXT, index, reinterpret_cast<LPARAM>(buffer));

				RECT rcText{ cbi.rcItem };
				::InflateRect(&rcText, -2, 0);

				constexpr DWORD dtFlags = DT_NOPREFIX | DT_LEFT | DT_VCENTER | DT_SINGLELINE;
				if (comboboxData.ensureTheme(hWnd))
				{
					DTTOPTS dtto{};
					dtto.dwSize = sizeof(DTTOPTS);
					dtto.dwFlags = DTT_TEXTCOLOR;
					dtto.crText = isDisabled ? DarkMode::getDisabledTextColor() : DarkMode::getTextColor();

					::DrawThemeTextEx(comboboxData._hTheme, hdc, CP_DROPDOWNITEM, isDisabled ? CBXSR_DISABLED : CBXSR_NORMAL, buffer, -1, dtFlags, &rcText, &dtto);
				}
				else
				{
					::SetTextColor(hdc, isDisabled ? DarkMode::getDisabledTextColor() : DarkMode::getTextColor());
					::DrawText(hdc, buffer, -1, &rcText, dtFlags);
				}
				delete[] buffer;
			}

			hasFocus = ::GetFocus() == hWnd;
			if (!isDisabled && hasFocus && ::SendMessage(hWnd, CB_GETDROPPEDSTATE, 0, 0) == FALSE)
			{
				::DrawFocusRect(hdc, &cbi.rcItem);
			}
		}
		else if (comboboxData._cbStyle == CBS_DROPDOWN && cbi.hwndItem != nullptr)
		{
			hasFocus = ::GetFocus() == cbi.hwndItem;

			::FillRect(hdc, &rcArrow, hSelectedBrush);
		}

		const auto hSelectedPen = isDisabled ? DarkMode::getDisabledEdgePen() : ((isHot || hasFocus) ? DarkMode::getHotEdgePen() : DarkMode::getEdgePen());
		auto holdPen = static_cast<HPEN>(::SelectObject(hdc, hSelectedPen));

		if (comboboxData._cbStyle != CBS_SIMPLE)
		{
			if (comboboxData.ensureTheme(hWnd))
			{
				RECT rcThemedArrow { rcArrow.left, rcArrow.top - 1, rcArrow.right, rcArrow.bottom - 1 };
				::DrawThemeBackground(comboboxData._hTheme, hdc, CP_DROPDOWNBUTTONRIGHT, isDisabled ? CBXSR_DISABLED : CBXSR_NORMAL, &rcThemedArrow, nullptr);
			}
			else
			{
				const auto clrText = isDisabled ? DarkMode::getDisabledTextColor() : (isHot ? DarkMode::getTextColor() : DarkMode::getDarkerTextColor());
				::SetTextColor(hdc, clrText);
				wchar_t arrow[] = L"˅";
				::DrawText(hdc, arrow, -1, &rcArrow, DT_NOPREFIX | DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);
			}
		}

		if (comboboxData._cbStyle == CBS_DROPDOWNLIST)
		{
			::ExcludeClipRect(hdc, rcClient.left + 1, rcClient.top + 1, rcClient.right - 1, rcClient.bottom - 1);
		}
		else if (comboboxData._cbStyle == CBS_DROPDOWN)
		{
			POINT edge[] = {
				{rcArrow.left - 1, rcArrow.top},
				{rcArrow.left - 1, rcArrow.bottom}
			};

			::Polyline(hdc, edge, _countof(edge));

			::ExcludeClipRect(hdc, cbi.rcItem.left, cbi.rcItem.top, cbi.rcItem.right, cbi.rcItem.bottom);
			::ExcludeClipRect(hdc, rcArrow.left - 1, rcArrow.top, rcArrow.right, rcArrow.bottom);

			HPEN hPen = ::CreatePen(PS_SOLID, 1, isDisabled ? DarkMode::getDlgBackgroundColor() : DarkMode::getBackgroundColor());
			RECT rcInner{ rcClient };
			::InflateRect(&rcInner, -1, -1);
			rcInner.right = rcArrow.left - 1;
			DarkMode::paintRoundFrameRect(hdc, rcInner, hPen);
			::DeleteObject(hPen);
			::InflateRect(&rcInner, -1, -1);
			::FillRect(hdc, &rcInner, DarkMode::getControlBackgroundBrush());
		}

		//const int roundCornerValue = DarkMode::isWindows11() ? 4 : 0;

		DarkMode::paintRoundFrameRect(hdc, rcClient, hSelectedPen);

		::SelectObject(hdc, holdPen);
	}

	constexpr UINT_PTR g_comboBoxSubclassID = 42;

	static LRESULT CALLBACK ComboBoxSubclass(
		HWND hWnd,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam,
		UINT_PTR uIdSubclass,
		DWORD_PTR dwRefData
	)
	{
		auto pComboboxData = reinterpret_cast<ComboboxData*>(dwRefData);

		switch (uMsg)
		{
			case WM_NCDESTROY:
			{
				::RemoveWindowSubclass(hWnd, ComboBoxSubclass, uIdSubclass);
				delete pComboboxData;
				break;
			}

			case WM_ERASEBKGND:
			{
				if (DarkMode::isEnabled() && pComboboxData->ensureTheme(hWnd))
				{
					auto hdc = reinterpret_cast<HDC>(wParam);
					if (pComboboxData->_cbStyle != CBS_DROPDOWN && hdc != pComboboxData->_hMemDC)
					{
						return FALSE;
					}
					return TRUE;
				}
				break;
			}

			case WM_PAINT:
			{
				if (!DarkMode::isEnabled())
				{
					break;
				}

				PAINTSTRUCT ps{};
				auto hdc = ::BeginPaint(hWnd, &ps);

				if (pComboboxData->_cbStyle != CBS_DROPDOWN)
				{
					if (ps.rcPaint.right <= ps.rcPaint.left || ps.rcPaint.bottom <= ps.rcPaint.top)
					{
						::EndPaint(hWnd, &ps);
						return 0;
					}

					RECT rcClient{};
					::GetClientRect(hWnd, &rcClient);

					if (pComboboxData->ensureBuffer(hdc, rcClient))
					{
						int savedState = ::SaveDC(pComboboxData->_hMemDC);
						::IntersectClipRect(
							pComboboxData->_hMemDC,
							ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right, ps.rcPaint.bottom
						);

						DarkMode::paintCombobox(hWnd, pComboboxData->_hMemDC, *pComboboxData);

						::RestoreDC(pComboboxData->_hMemDC, savedState);

						::BitBlt(
							hdc,
							ps.rcPaint.left, ps.rcPaint.top,
							ps.rcPaint.right - ps.rcPaint.left,
							ps.rcPaint.bottom - ps.rcPaint.top,
							pComboboxData->_hMemDC,
							ps.rcPaint.left, ps.rcPaint.top,
							SRCCOPY
						);
					}
				}
				else
				{
					DarkMode::paintCombobox(hWnd, hdc, *pComboboxData);
				}

				::EndPaint(hWnd, &ps);
				return 0;
			}

			case WM_DPICHANGED:
			{
				pComboboxData->closeTheme();
				return 0;
			}

			case WM_THEMECHANGED:
			{
				pComboboxData->closeTheme();
				break;
			}
		}
		return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	void subclassComboBoxControl(HWND hWnd)
	{
		if (::GetWindowSubclass(hWnd, ComboBoxSubclass, g_comboBoxSubclassID, nullptr) == FALSE)
		{
			auto cbStyle = ::GetWindowLongPtr(hWnd, GWL_STYLE) & CBS_DROPDOWNLIST;
			auto pComboboxData = reinterpret_cast<DWORD_PTR>(new ComboboxData(cbStyle));
			::SetWindowSubclass(hWnd, ComboBoxSubclass, g_comboBoxSubclassID, pComboboxData);
		}
	}

	constexpr UINT_PTR g_comboboxExSubclassID = 42;

	static LRESULT CALLBACK ComboboxExSubclass(
		HWND hWnd,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam,
		UINT_PTR uIdSubclass,
		DWORD_PTR /*dwRefData*/
	)
	{
		switch (uMsg)
		{
			case WM_NCDESTROY:
			{
				::RemoveWindowSubclass(hWnd, ComboboxExSubclass, uIdSubclass);
				DarkMode::unhookSysColor();
				break;
			}

			case WM_COMMAND:
			{
				// ComboboxEx has only one child combobox, so only control-defined notification code is checked.
				// Hooking is done only when listbox is about to show. And unhook when listbox is closed.
				// This process is used to avoid visual glitches in other GUI.
				switch (HIWORD(wParam))
				{
					case CBN_DROPDOWN:
					{
						DarkMode::hookSysColor();
						break;
					}

					case CBN_CLOSEUP:
					{
						DarkMode::unhookSysColor();
						break;
					}

					default:
						break;
				}
				break;
			}
		}
		return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	static void subclassComboboxEx(HWND hWnd)
	{
		if (::GetWindowSubclass(hWnd, ComboboxExSubclass, g_comboboxExSubclassID, nullptr) == FALSE)
		{
			::SetWindowSubclass(hWnd, ComboboxExSubclass, g_comboboxExSubclassID, 0);
		}
	}

	void subclassComboboxEx(HWND hWnd, DarkModeParams p)
	{
		if (p._subclass)
		{
			subclassComboboxEx(hWnd);
		}
	}

	constexpr UINT_PTR g_listViewSubclassID = 42;

	static LRESULT CALLBACK ListViewSubclass(
		HWND hWnd,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam,
		UINT_PTR uIdSubclass,
		DWORD_PTR /*dwRefData*/
	)
	{
		switch (uMsg)
		{
			case WM_NCDESTROY:
			{
				DarkMode::unhookSysColor();
				::RemoveWindowSubclass(hWnd, ListViewSubclass, uIdSubclass);
				break;
			}

			case WM_PAINT:
			{
				const auto lvStyle = ::GetWindowLongPtr(hWnd, GWL_STYLE) & LVS_TYPEMASK;
				const bool isReport = (lvStyle == LVS_REPORT);
				bool hasGridlines = false;
				if (isReport)
				{
					const auto lvExStyle = ListView_GetExtendedListViewStyle(hWnd);
					hasGridlines = (lvExStyle & LVS_EX_GRIDLINES) == LVS_EX_GRIDLINES;
				}

				if (hasGridlines)
				{
					DarkMode::hookSysColor();
					LRESULT lr = ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
					DarkMode::unhookSysColor();
					return lr;
				}
				break;
			}

			// For edit control, which is created when renaming/editing items
			case WM_CTLCOLOREDIT:
			{
				if (DarkMode::isEnabled())
				{
					return DarkMode::onCtlColorControl(reinterpret_cast<HDC>(wParam));
				}
				break;
			}

			case WM_NOTIFY:
			{
				switch (reinterpret_cast<LPNMHDR>(lParam)->code)
				{
					case NM_CUSTOMDRAW:
					{
						auto lpnmcd = reinterpret_cast<LPNMCUSTOMDRAW>(lParam);
						switch (lpnmcd->dwDrawStage)
						{
							case CDDS_PREPAINT:
							{
								if (DarkMode::isExperimentalActive())
								{
									return CDRF_NOTIFYITEMDRAW;
								}
								return CDRF_DODEFAULT;
							}

							case CDDS_ITEMPREPAINT:
							{
								::SetTextColor(lpnmcd->hdc, DarkMode::getDarkerTextColor());

								return CDRF_NEWFONT;
							}

							default:
								return CDRF_DODEFAULT;
						}
					}
					break;
				}
				break;
			}
			break;
		}
		return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	static void subclassListViewControl(HWND hWnd)
	{
		if (::GetWindowSubclass(hWnd, ListViewSubclass, g_listViewSubclassID, nullptr) == FALSE)
		{
			::SetWindowSubclass(hWnd, ListViewSubclass, g_listViewSubclassID, 0);
		}
	}

	struct HeaderData : public BaseControlBufferData
	{
		HFONT _hFont = nullptr;
		bool _isHot = false;
		bool _hasBtnStyle = true;
		POINT _pt{ LONG_MIN, LONG_MIN };
		bool _isPressed = false;

		HeaderData() = default;

		HeaderData(bool hasBtnStyle)
			: _hasBtnStyle(hasBtnStyle) {}

		~HeaderData() override
		{
			destroyFont();
		}

		const wchar_t* getThemeClass() const override
		{
			return VSCLASS_HEADER;
		}

		void setFont(const HFONT& hFont)
		{
			destroyFont();
			this->_hFont = hFont;
		}

		void destroyFont()
		{
			if (_hFont != nullptr)
			{
				::DeleteObject(_hFont);
				_hFont = nullptr;
			}
		}
	};

	void paintHeader(HWND hWnd, HDC hdc, HeaderData& headerData)
	{
		::SetBkMode(hdc, TRANSPARENT);
		auto hOldPen = static_cast<HPEN>(::SelectObject(hdc, DarkMode::getHeaderEdgePen()));

		RECT rcHeader{};
		::GetClientRect(hWnd, &rcHeader);
		::FillRect(hdc, &rcHeader, DarkMode::getHeaderBackgroundBrush());

		const bool hasTheme = headerData.ensureTheme(hWnd);

		LOGFONT lf{};
		if (headerData._hFont == nullptr
			&& hasTheme
			&& SUCCEEDED(::GetThemeFont(headerData._hTheme, hdc, HP_HEADERITEM, HIS_NORMAL, TMT_FONT, &lf)))
		{
			headerData._hFont = ::CreateFontIndirect(&lf);
		}

		HFONT hFont = (headerData._hFont == nullptr) ? reinterpret_cast<HFONT>(::SendMessage(hWnd, WM_GETFONT, 0, 0)) : headerData._hFont;
		auto hOldFont = static_cast<HFONT>(::SelectObject(hdc, hFont));

		DTTOPTS dtto{};
		if (hasTheme)
		{
			dtto.dwSize = sizeof(DTTOPTS);
			dtto.dwFlags = DTT_TEXTCOLOR;
			dtto.crText = DarkMode::getHeaderTextColor();
		}
		else
		{
			::SetTextColor(hdc, DarkMode::getHeaderTextColor());
		}

		auto hList = ::GetParent(hWnd);
		const auto lvStyle = ::GetWindowLongPtr(hList, GWL_STYLE) & LVS_TYPEMASK;
		bool hasGridlines = false;
		if (lvStyle == LVS_REPORT)
		{
			const auto lvExStyle = ListView_GetExtendedListViewStyle(hList);
			hasGridlines = (lvExStyle & LVS_EX_GRIDLINES) == LVS_EX_GRIDLINES;
		}

		const int count = Header_GetItemCount(hWnd);
		RECT rcItem{};
		for (int i = 0; i < count; i++)
		{
			Header_GetItemRect(hWnd, i, &rcItem);
			const bool isOnItem = ::PtInRect(&rcItem, headerData._pt);

			if (headerData._hasBtnStyle && isOnItem)
			{
				RECT rcTmp{ rcItem };
				if (hasGridlines)
				{
					::OffsetRect(&rcTmp, 1, 0);
				}
				else if (DarkMode::isExperimentalActive())
				{
					::OffsetRect(&rcTmp, -1, 0);
				}
				::FillRect(hdc, &rcTmp, DarkMode::getHeaderHotBackgroundBrush());
			}

			wchar_t buffer[MAX_PATH]{};
			HDITEM hdi{};
			hdi.mask = HDI_TEXT | HDI_FORMAT;
			hdi.pszText = buffer;
			hdi.cchTextMax = MAX_PATH - 1;

			Header_GetItem(hWnd, i, &hdi);

			if (hasTheme
				&& ((hdi.fmt & HDF_SORTUP) == HDF_SORTUP
					|| (hdi.fmt & HDF_SORTDOWN) == HDF_SORTDOWN))
			{
				int iStateID = ((hdi.fmt & HDF_SORTUP) == HDF_SORTUP) ? HSAS_SORTEDUP : HSAS_SORTEDDOWN;
				RECT rcArrow{ rcItem };
				SIZE szArrow{};
				if (SUCCEEDED(::GetThemePartSize(headerData._hTheme, hdc, HP_HEADERSORTARROW, iStateID, nullptr, TS_DRAW, &szArrow)))
					rcArrow.bottom = szArrow.cy;

				::DrawThemeBackground(headerData._hTheme, hdc, HP_HEADERSORTARROW, iStateID, &rcArrow, nullptr);
			}

			LONG edgeX = rcItem.right;
			if (!hasGridlines)
			{
				edgeX--;
				if (DarkMode::isExperimentalActive())
					edgeX--;
			}

			POINT edge[]{
				{edgeX, rcItem.top},
				{edgeX, rcItem.bottom}
			};
			::Polyline(hdc, edge, _countof(edge));

			int dtFlags = DT_VCENTER | DT_SINGLELINE | DT_WORD_ELLIPSIS | DT_HIDEPREFIX;
			if ((hdi.fmt & HDF_RIGHT) == HDF_RIGHT)
				dtFlags |= DT_RIGHT;
			else if ((hdi.fmt & HDF_CENTER) == HDF_CENTER)
				dtFlags |= DT_CENTER;

			rcItem.left += 6;
			rcItem.right -= 8;

			if (headerData._isPressed && isOnItem)
				::OffsetRect(&rcItem, 1, 1);

			if (hasTheme)
				::DrawThemeTextEx(headerData._hTheme, hdc, HP_HEADERITEM, HIS_NORMAL, hdi.pszText, -1, dtFlags, &rcItem, &dtto);
			else
				::DrawText(hdc, hdi.pszText, -1, &rcItem, dtFlags);
		}

		::SelectObject(hdc, hOldFont);
		::SelectObject(hdc, hOldPen);
	}

	constexpr UINT_PTR g_headerSubclassID = 42;

	static LRESULT CALLBACK HeaderSubclass(
		HWND hWnd,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam,
		UINT_PTR uIdSubclass,
		DWORD_PTR dwRefData
	)
	{
		auto pHeaderData = reinterpret_cast<HeaderData*>(dwRefData);

		switch (uMsg)
		{
			case WM_NCDESTROY:
			{
				::RemoveWindowSubclass(hWnd, HeaderSubclass, uIdSubclass);
				delete pHeaderData;
				break;
			}

			case WM_ERASEBKGND:
			{
				if (DarkMode::isEnabled() && pHeaderData->ensureTheme(hWnd))
				{
					auto hdc = reinterpret_cast<HDC>(wParam);
					if (hdc != pHeaderData->_hMemDC)
					{
						return FALSE;
					}
					return TRUE;
				}
				break;
			}

			case WM_PAINT:
			{
				if (!DarkMode::isEnabled())
				{
					break;
				}

				PAINTSTRUCT ps{};
				auto hdc = ::BeginPaint(hWnd, &ps);

				if (ps.rcPaint.right <= ps.rcPaint.left || ps.rcPaint.bottom <= ps.rcPaint.top)
				{
					::EndPaint(hWnd, &ps);
					return 0;
				}

				RECT rcClient{};
				::GetClientRect(hWnd, &rcClient);

				if (pHeaderData->ensureBuffer(hdc, rcClient))
				{
					int savedState = ::SaveDC(pHeaderData->_hMemDC);
					::IntersectClipRect(
						pHeaderData->_hMemDC,
						ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right, ps.rcPaint.bottom
					);

					DarkMode::paintHeader(hWnd, pHeaderData->_hMemDC, *pHeaderData);

					::RestoreDC(pHeaderData->_hMemDC, savedState);

					::BitBlt(
						hdc,
						ps.rcPaint.left, ps.rcPaint.top,
						ps.rcPaint.right - ps.rcPaint.left,
						ps.rcPaint.bottom - ps.rcPaint.top,
						pHeaderData->_hMemDC,
						ps.rcPaint.left, ps.rcPaint.top,
						SRCCOPY
					);
				}

				::EndPaint(hWnd, &ps);
				return 0;
			}

			case WM_DPICHANGED:
			{
				pHeaderData->closeTheme();
				return 0;
			}

			case WM_THEMECHANGED:
			{
				pHeaderData->closeTheme();
				break;
			}

			case WM_LBUTTONDOWN:
			{
				if (!pHeaderData->_hasBtnStyle)
					break;

				pHeaderData->_isPressed = true;

				break;
			}

			case WM_LBUTTONUP:
			{
				if (!pHeaderData->_hasBtnStyle)
					break;

				pHeaderData->_isPressed = false;
				break;
			}

			case WM_MOUSEMOVE:
			{
				if (!pHeaderData->_hasBtnStyle || pHeaderData->_isPressed)
					break;

				TRACKMOUSEEVENT tme{};

				if (!pHeaderData->_isHot)
				{
					tme.cbSize = sizeof(TRACKMOUSEEVENT);
					tme.dwFlags = TME_LEAVE;
					tme.hwndTrack = hWnd;

					::TrackMouseEvent(&tme);

					pHeaderData->_isHot = true;
				}

				pHeaderData->_pt.x = GET_X_LPARAM(lParam);
				pHeaderData->_pt.y = GET_Y_LPARAM(lParam);

				::InvalidateRect(hWnd, nullptr, FALSE);
				break;
			}

			case WM_MOUSELEAVE:
			{
				if (!pHeaderData->_hasBtnStyle)
					break;

				LRESULT result = ::DefSubclassProc(hWnd, uMsg, wParam, lParam);

				pHeaderData->_isHot = false;
				pHeaderData->_pt.x = LONG_MIN;
				pHeaderData->_pt.y = LONG_MIN;

				::InvalidateRect(hWnd, nullptr, TRUE);

				return result;
			}
		}
		return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	void subclassHeaderControl(HWND hWnd)
	{
		if (::GetWindowSubclass(hWnd, HeaderSubclass, g_headerSubclassID, nullptr) == FALSE)
		{
			const bool hasBtnStyle = (::GetWindowLongPtr(hWnd, GWL_STYLE) & HDS_BUTTONS) == HDS_BUTTONS;
			auto pHeaderData = reinterpret_cast<DWORD_PTR>(new HeaderData(hasBtnStyle));
			::SetWindowSubclass(hWnd, HeaderSubclass, g_headerSubclassID, pHeaderData);
		}
	}

	struct UpDownData : public BaseControlThemeData
	{
		bool _isHorizontal = false;
		RECT _rcClient{};
		RECT _rcPrev{};
		RECT _rcNext{};
		bool _wasHotNext = false;

		UpDownData() = default;

		UpDownData(HWND hWnd)
		{
			const auto nStyle = ::GetWindowLongPtr(hWnd, GWL_STYLE);
			_isHorizontal = ((nStyle & UDS_HORZ) == UDS_HORZ);

			updateRect(hWnd);
		}

		~UpDownData() override = default;

		const wchar_t* getThemeClass() const override
		{
			return VSCLASS_BUTTON;
		}

		void updateRect(HWND hWnd)
		{
			::GetClientRect(hWnd, &_rcClient);

			if (_isHorizontal)
			{
				RECT rcArrowLeft{
					_rcClient.left, _rcClient.top,
					_rcClient.right - ((_rcClient.right - _rcClient.left) / 2), _rcClient.bottom
				};

				RECT rcArrowRight{
					rcArrowLeft.right, _rcClient.top,
					_rcClient.right, _rcClient.bottom
				};

				_rcPrev = rcArrowLeft;
				_rcNext = rcArrowRight;
			}
			else
			{
				_rcClient.left += 2;

				RECT rcArrowTop{
					_rcClient.left, _rcClient.top,
					_rcClient.right, _rcClient.bottom - ((_rcClient.bottom - _rcClient.top) / 2)
				};

				RECT rcArrowBottom{
					_rcClient.left, rcArrowTop.bottom,
					_rcClient.right, _rcClient.bottom
				};

				_rcPrev = rcArrowTop;
				_rcNext = rcArrowBottom;
			}
		}
	};

	static void paintUpDown(HWND hWnd, HDC hdc, UpDownData& upDownData)
	{
		const bool isDisabled = ::IsWindowEnabled(hWnd) == FALSE;
		const bool hasTheme = upDownData.ensureTheme(hWnd) && upDownData._isHorizontal;
		
		::FillRect(hdc, &upDownData._rcClient, DarkMode::getDlgBackgroundBrush());
		::SetBkMode(hdc, TRANSPARENT);

		POINT ptCursor{};
		::GetCursorPos(&ptCursor);
		::ScreenToClient(hWnd, &ptCursor);

		const bool isHotPrev = ::PtInRect(&upDownData._rcPrev, ptCursor);
		const bool isHotNext = ::PtInRect(&upDownData._rcNext, ptCursor);

		upDownData._wasHotNext = isHotNext;

		if (hasTheme)
		{
			auto stateID = isDisabled ? PBS_DISABLED : PBS_NORMAL;
			::DrawThemeBackground(upDownData._hTheme, hdc, BP_PUSHBUTTON, isHotPrev ? PBS_HOT : stateID, &upDownData._rcPrev, nullptr);
			::DrawThemeBackground(upDownData._hTheme, hdc, BP_PUSHBUTTON, isHotNext ? PBS_HOT : stateID, &upDownData._rcNext, nullptr);
		}
		else
		{
			HBRUSH hBrush = isDisabled ? DarkMode::getDlgBackgroundBrush() : DarkMode::getControlBackgroundBrush();
			::FillRect(hdc, &upDownData._rcPrev, isHotPrev ? DarkMode::getHotBackgroundBrush() : hBrush);
			::FillRect(hdc, &upDownData._rcNext, isHotNext ? DarkMode::getHotBackgroundBrush() : hBrush);
		}

		constexpr auto arrowTextFlags = DT_NOPREFIX | DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP;
		COLORREF clrText = isDisabled ? DarkMode::getDisabledTextColor() : DarkMode::getDarkerTextColor();

		::SetTextColor(hdc, isHotPrev ? DarkMode::getTextColor() : clrText);
		::DrawText(hdc, upDownData._isHorizontal ? L"<" : L"˄", -1, &upDownData._rcPrev, arrowTextFlags);

		::SetTextColor(hdc, isHotNext ? DarkMode::getTextColor() : clrText);
		::DrawText(hdc, upDownData._isHorizontal ? L">" : L"˅", -1, &upDownData._rcNext, arrowTextFlags);

		if (!hasTheme)
		{
			HPEN hPen = isDisabled ? DarkMode::getDisabledEdgePen() : DarkMode::getEdgePen();
			DarkMode::paintRoundFrameRect(hdc, upDownData._rcPrev, hPen);
			DarkMode::paintRoundFrameRect(hdc, upDownData._rcNext, hPen);
		}
	}

	constexpr UINT_PTR g_upDownSubclassID = 42;

	static LRESULT CALLBACK UpDownSubclass(
		HWND hWnd,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam,
		UINT_PTR uIdSubclass,
		DWORD_PTR dwRefData
	)
	{
		auto pUpDownData = reinterpret_cast<UpDownData*>(dwRefData);

		switch (uMsg)
		{
			case WM_NCDESTROY:
			{
				::RemoveWindowSubclass(hWnd, UpDownSubclass, uIdSubclass);
				delete pUpDownData;
				break;
			}

			case WM_ERASEBKGND:
			{
				if (DarkMode::isEnabled())
				{
					return TRUE;
				}
				break;
			}

			case WM_PRINTCLIENT:
			case WM_PAINT:
			{
				if (!DarkMode::isEnabled())
				{
					break;
				}

				PAINTSTRUCT ps{};
				auto hdc = ::BeginPaint(hWnd, &ps);

				DarkMode::paintUpDown(hWnd, hdc, *pUpDownData);

				::EndPaint(hWnd, &ps);
				return 0;
			}

			case WM_DPICHANGED:
			{
				pUpDownData->closeTheme();
				pUpDownData->updateRect(hWnd);
				return 0;
			}

			case WM_THEMECHANGED:
			{
				pUpDownData->closeTheme();
				break;
			}

			case WM_MOUSEMOVE:
			{
				if (!DarkMode::isEnabled())
				{
					break;
				}

				POINT ptCursor{};
				::GetCursorPos(&ptCursor);
				::ScreenToClient(hWnd, &ptCursor);

				const bool isHotPrev = ::PtInRect(&pUpDownData->_rcPrev, ptCursor);

				if ((isHotPrev && pUpDownData->_wasHotNext))
				{
					pUpDownData->_wasHotNext = false;
					::RedrawWindow(hWnd, &pUpDownData->_rcNext, nullptr, RDW_INVALIDATE);
				}

				break;
			}

			case WM_MOUSELEAVE:
			{
				if (!DarkMode::isEnabled())
				{
					break;
				}

				pUpDownData->_wasHotNext = false;
				::RedrawWindow(hWnd, nullptr, nullptr, RDW_INVALIDATE);

				break;
			}
		}
		return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	static void subclassAndThemeUpDownControl(HWND hWnd, DarkModeParams p)
	{
		if (p._subclass && ::GetWindowSubclass(hWnd, UpDownSubclass, g_upDownSubclassID, nullptr) == FALSE)
		{
			auto pUpDownData = reinterpret_cast<DWORD_PTR>(new UpDownData(hWnd));
			::SetWindowSubclass(hWnd, UpDownSubclass, g_upDownSubclassID, pUpDownData);
		}

		if (p._theme)
		{
			::SetWindowTheme(hWnd, p._themeClassName, nullptr);
		}
	}

	bool subclassTabUpDownControl(HWND hWnd)
	{
		if (::GetWindowSubclass(hWnd, UpDownSubclass, g_upDownSubclassID, nullptr) == FALSE)
		{
			if (cmpWndClassName(hWnd, UPDOWN_CLASS))
			{
				auto pUpDownData = reinterpret_cast<DWORD_PTR>(new UpDownData(hWnd));
				::SetWindowSubclass(hWnd, UpDownSubclass, g_upDownSubclassID, pUpDownData);
				DarkMode::setDarkExplorerTheme(hWnd);
				return true;
			}
		}
		return false;
	}

	struct StatusBarData : public BaseControlBufferData
	{
		HFONT _hFont = nullptr;

		StatusBarData() = default;
		StatusBarData(const HFONT& hFont) : _hFont(hFont) {}

		~StatusBarData() override
		{
			destroyFont();
		}

		const wchar_t* getThemeClass() const override
		{
			return VSCLASS_STATUS;
		}

		void setFont(const HFONT& hFont)
		{
			destroyFont();
			_hFont = hFont;
		}

		void destroyFont()
		{
			if (_hFont != nullptr)
			{
				::DeleteObject(_hFont);
				_hFont = nullptr;
			}
		}
	};

	static void paintStatusBar(HWND hWnd, HDC hdc, StatusBarData& statusBarData)
	{
		struct {
			int horizontal = 0;
			int vertical = 0;
			int between = 0;
		} borders{};

		::SendMessage(hWnd, SB_GETBORDERS, 0, reinterpret_cast<LPARAM>(&borders));

		const auto nStyle = ::GetWindowLongPtr(hWnd, GWL_STYLE);
		const bool hasSizeGrip = (nStyle & SBARS_SIZEGRIP) == SBARS_SIZEGRIP;

		auto holdPen = static_cast<HPEN>(::SelectObject(hdc, DarkMode::getEdgePen()));
		auto holdFont = static_cast<HFONT>(::SelectObject(hdc, statusBarData._hFont));

		::SetBkMode(hdc, TRANSPARENT);
		::SetTextColor(hdc, DarkMode::getTextColor());

		RECT rcClient{};
		::GetClientRect(hWnd, &rcClient);

		::FillRect(hdc, &rcClient, DarkMode::getBackgroundBrush());

		auto nParts = static_cast<int>(::SendMessage(hWnd, SB_GETPARTS, 0, 0));
		std::wstring str;
		RECT rcPart{};
		RECT rcIntersect{};
		for (int i = 0; i < nParts; ++i) {
			::SendMessage(hWnd, SB_GETRECT, i, reinterpret_cast<LPARAM>(&rcPart));
			if (::IntersectRect(&rcIntersect, &rcPart, &rcClient) == 0)
			{
				continue;
			}

			if (i < nParts - 0 && !(nParts < 2 && hasSizeGrip))
			{
				POINT edges[]{
					{rcPart.right - borders.between, rcPart.top + 1},
					{rcPart.right - borders.between, rcPart.bottom - 3}
				};
				::Polyline(hdc, edges, _countof(edges));
			}

			DWORD cchText = 0;
			cchText = LOWORD(::SendMessage(hWnd, SB_GETTEXTLENGTH, i, 0));
			str.resize(cchText + 1); // technically the std::wstring might not have an internal null character at the end of the buffer, so add one
			LRESULT lr = ::SendMessage(hWnd, SB_GETTEXT, i, reinterpret_cast<LPARAM>(&str[0]));
			str.resize(cchText); // remove the extra NULL character
			bool ownerDraw = false;
			if (cchText == 0 && (lr & ~(SBT_NOBORDERS | SBT_POPOUT | SBT_RTLREADING)) != 0)
			{
				// this is a pointer to the text
				ownerDraw = true;
			}

			rcPart.left += borders.between;
			rcPart.right -= borders.vertical;

			if (ownerDraw)
			{
				UINT id = ::GetDlgCtrlID(hWnd);
				DRAWITEMSTRUCT dis{
					0
					, 0
					, static_cast<UINT>(i)
					, ODA_DRAWENTIRE
					, id
					, hWnd
					, hdc
					, rcPart
					, static_cast<ULONG_PTR>(lr)
				};

				::SendMessage(::GetParent(hWnd), WM_DRAWITEM, id, reinterpret_cast<LPARAM>(&dis));
			}
			else
			{
				::DrawText(hdc, str.data(), static_cast<int>(str.size()), &rcPart, DT_SINGLELINE | DT_VCENTER | DT_LEFT);
			}
		}

		/*POINT edgeHor[] = {
			{rcClient.left, rcClient.top},
			{rcClient.right, rcClient.top}
		};
		Polyline(hdc, edgeHor, _countof(edgeHor));*/

		if (hasSizeGrip)
		{
			statusBarData.ensureTheme(hWnd);
			SIZE gripSize{};
			::GetThemePartSize(statusBarData._hTheme, hdc, SP_GRIPPER, 0, &rcClient, TS_DRAW, &gripSize);
			RECT rc = rcClient;
			rc.left = rc.right - gripSize.cx;
			rc.top = rc.bottom - gripSize.cy;
			::DrawThemeBackground(statusBarData._hTheme, hdc, SP_GRIPPER, 0, &rc, nullptr);
		}

		::SelectObject(hdc, holdFont);
		::SelectObject(hdc, holdPen);
	}

	constexpr UINT_PTR g_statusBarSubclassID = 42;

	static LRESULT CALLBACK StatusBarSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
	{
		auto pStatusBarData = reinterpret_cast<StatusBarData*>(dwRefData);

		switch (uMsg)
		{
			case WM_NCDESTROY:
			{
				::RemoveWindowSubclass(hWnd, StatusBarSubclass, uIdSubclass);
				delete pStatusBarData;
				break;
			}

			case WM_ERASEBKGND:
			{
				if (DarkMode::isEnabled() && pStatusBarData->ensureTheme(hWnd))
				{
					auto hdc = reinterpret_cast<HDC>(wParam);
					if (hdc != pStatusBarData->_hMemDC)
					{
						return FALSE;
					}
					return TRUE;
				}
				break;
			}

			case WM_PAINT:
			{
				if (DarkMode::isEnabled() && pStatusBarData->ensureTheme(hWnd))
				{
					PAINTSTRUCT ps{};
					auto hdc = ::BeginPaint(hWnd, &ps);

					if (ps.rcPaint.right <= ps.rcPaint.left || ps.rcPaint.bottom <= ps.rcPaint.top)
					{
						::EndPaint(hWnd, &ps);
						return 0;
					}

					RECT rcClient{};
					::GetClientRect(hWnd, &rcClient);

					if (pStatusBarData->ensureBuffer(hdc, rcClient))
					{
						int savedState = ::SaveDC(pStatusBarData->_hMemDC);
						::IntersectClipRect(
							pStatusBarData->_hMemDC,
							ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right, ps.rcPaint.bottom
						);

						DarkMode::paintStatusBar(hWnd, pStatusBarData->_hMemDC, *pStatusBarData);

						::RestoreDC(pStatusBarData->_hMemDC, savedState);

						::BitBlt(
							hdc,
							ps.rcPaint.left, ps.rcPaint.top,
							ps.rcPaint.right - ps.rcPaint.left,
							ps.rcPaint.bottom - ps.rcPaint.top,
							pStatusBarData->_hMemDC,
							ps.rcPaint.left, ps.rcPaint.top,
							SRCCOPY
						);
					}

					::EndPaint(hWnd, &ps);
					return 0;
				}
				break;
			}

			case WM_DPICHANGED:
			case WM_THEMECHANGED:
			{
				pStatusBarData->closeTheme();

				LOGFONT lf{};
				NONCLIENTMETRICS ncm{};
				ncm.cbSize = sizeof(NONCLIENTMETRICS);
				if (::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0) != FALSE)
				{
					lf = ncm.lfStatusFont;
					pStatusBarData->setFont(::CreateFontIndirect(&lf));
				}

				if (uMsg != WM_THEMECHANGED)
				{
					return 0;
				}
				break;
			}
		}
		return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	void subclassStatusBarControl(HWND hWnd)
	{
		if (::GetWindowSubclass(hWnd, StatusBarSubclass, g_statusBarSubclassID, nullptr) == FALSE)
		{
			LOGFONT lf{};
			NONCLIENTMETRICS ncm{};
			ncm.cbSize = sizeof(NONCLIENTMETRICS);
			if (::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0) != FALSE)
			{
				lf = ncm.lfStatusFont;
			}
			auto pStatusBarData = reinterpret_cast<DWORD_PTR>(new StatusBarData(::CreateFontIndirect(&lf)));
			::SetWindowSubclass(hWnd, StatusBarSubclass, g_statusBarSubclassID, pStatusBarData);
		}
	}

	void subclassStatusBarControl(HWND hWnd, DarkModeParams p)
	{
		if (p._subclass)
		{
			DarkMode::subclassStatusBarControl(hWnd);
		}
	}

	struct ProgressBarData : public BaseControlBufferData
	{
		int _iStateID = PBFS_PARTIAL; // PBFS_PARTIAL for cyan color

		ProgressBarData() = default;

		~ProgressBarData() override = default;

		const wchar_t* getThemeClass() const override
		{
			return VSCLASS_PROGRESS;
		}
	};

	static void getProgressBarRects(HWND hWnd, RECT* rcEmpty, RECT* rcFilled)
	{
		auto pos = static_cast<int>(::SendMessage(hWnd, PBM_GETPOS, 0, 0));

		PBRANGE range{};
		::SendMessage(hWnd, PBM_GETRANGE, TRUE, reinterpret_cast<LPARAM>(&range));
		int min = range.iLow;
		int max = range.iHigh;

		int currPos = pos - min;
		if (currPos != 0)
		{
			int totalWidth = rcEmpty->right - rcEmpty->left;
			rcFilled->left = rcEmpty->left;
			rcFilled->top = rcEmpty->top;
			rcFilled->bottom = rcEmpty->bottom;
			rcFilled->right = rcEmpty->left + static_cast<int>(static_cast<double>(currPos) / (max - min) * totalWidth);

			rcEmpty->left = rcFilled->right; // to avoid painting under filled part
		}
	}

	static void paintProgressBar(HWND hWnd, HDC hdc, ProgressBarData& progressBarData)
	{
		RECT rcClient{};
		::GetClientRect(hWnd, &rcClient);

		DarkMode::paintRoundFrameRect(hdc, rcClient, DarkMode::getEdgePen(), 0, 0);

		::InflateRect(&rcClient, -1, -1);
		rcClient.left = 1;

		RECT rcFill{};
		DarkMode::getProgressBarRects(hWnd, &rcClient, &rcFill);
		::DrawThemeBackground(progressBarData._hTheme, hdc, PP_FILL, progressBarData._iStateID, &rcFill, NULL);
		::FillRect(hdc, &rcClient, DarkMode::getControlBackgroundBrush());
	}

	const UINT_PTR g_progressBarSubclassID = 42;

	static LRESULT CALLBACK ProgressBarSubclass(
		HWND hWnd,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam,
		UINT_PTR uIdSubclass,
		DWORD_PTR dwRefData
	)
	{
		auto pProgressBarData = reinterpret_cast<ProgressBarData*>(dwRefData);

		switch (uMsg)
		{
			case WM_NCDESTROY:
			{
				::RemoveWindowSubclass(hWnd, ProgressBarSubclass, uIdSubclass);
				delete pProgressBarData;
				break;
			}

			case WM_ERASEBKGND:
			{
				if (DarkMode::isEnabled() && pProgressBarData->ensureTheme(hWnd))
				{
					auto hdc = reinterpret_cast<HDC>(wParam);
					if (hdc != pProgressBarData->_hMemDC)
					{
						return FALSE;
					}
					return TRUE;
				}
				break;
			}

			case WM_PAINT:
			{
				if (DarkMode::isEnabled() && pProgressBarData->ensureTheme(hWnd))
				{
					PAINTSTRUCT ps{};
					auto hdc = ::BeginPaint(hWnd, &ps);

					if (ps.rcPaint.right <= ps.rcPaint.left || ps.rcPaint.bottom <= ps.rcPaint.top)
					{
						::EndPaint(hWnd, &ps);
						return 0;
					}

					RECT rcClient{};
					::GetClientRect(hWnd, &rcClient);

					if (pProgressBarData->ensureBuffer(hdc, rcClient))
					{
						int savedState = ::SaveDC(pProgressBarData->_hMemDC);
						::IntersectClipRect(
							pProgressBarData->_hMemDC,
							ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right, ps.rcPaint.bottom
						);

						DarkMode::paintProgressBar(hWnd, pProgressBarData->_hMemDC, *pProgressBarData);

						::RestoreDC(pProgressBarData->_hMemDC, savedState);

						::BitBlt(
							hdc,
							ps.rcPaint.left, ps.rcPaint.top,
							ps.rcPaint.right - ps.rcPaint.left,
							ps.rcPaint.bottom - ps.rcPaint.top,
							pProgressBarData->_hMemDC,
							ps.rcPaint.left, ps.rcPaint.top,
							SRCCOPY
						);
					}

					::EndPaint(hWnd, &ps);
					return 0;
				}
				break;
			}

			case WM_DPICHANGED:
			{
				pProgressBarData->closeTheme();
				return 0;
			}

			case WM_THEMECHANGED:
			{
				pProgressBarData->closeTheme();
				break;
			}

			case PBM_SETSTATE:
			{
				switch (wParam)
				{
					case PBST_NORMAL:
					{
						pProgressBarData->_iStateID = PBFS_NORMAL; // green
						break;
					}

					case PBST_ERROR:
					{
						pProgressBarData->_iStateID = PBFS_ERROR; // red
						break;
					}
					case PBST_PAUSED:
					{
						pProgressBarData->_iStateID = PBFS_PAUSED; // yellow
						break;
					}
				}
				break;
			}
		}
		return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	void subclassProgressBarControl(HWND hWnd)
	{
		if (::GetWindowSubclass(hWnd, ProgressBarSubclass, g_progressBarSubclassID, NULL) == FALSE)
		{
			DWORD_PTR pProgressBarData = reinterpret_cast<DWORD_PTR>(new ProgressBarData);
			::SetWindowSubclass(hWnd, ProgressBarSubclass, g_progressBarSubclassID, pProgressBarData);
		}
	}

	void subclassProgressBarControl(HWND hWnd, DarkModeParams p)
	{
		if (p._subclass)
		{
			DarkMode::subclassProgressBarControl(hWnd);
		}
	}

	struct StaticTextData
	{
		bool isEnabled = true;

		StaticTextData() = default;
		StaticTextData(HWND hWnd)
		{
			isEnabled = ::IsWindowEnabled(hWnd) == TRUE;
		}

		~StaticTextData() = default;
	};

	constexpr UINT_PTR g_staticTextSubclassID = 42;

	static LRESULT CALLBACK StaticTextSubclass(
		HWND hWnd,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam,
		UINT_PTR uIdSubclass,
		DWORD_PTR dwRefData
	)
	{
		auto pStaticTextData = reinterpret_cast<StaticTextData*>(dwRefData);

		switch (uMsg)
		{
			case WM_NCDESTROY:
			{
				::RemoveWindowSubclass(hWnd, StaticTextSubclass, uIdSubclass);
				delete pStaticTextData;
				break;
			}

			case WM_ENABLE:
			{
				pStaticTextData->isEnabled = (wParam == TRUE);

				const auto nStyle = ::GetWindowLongPtr(hWnd, GWL_STYLE);
				if (!pStaticTextData->isEnabled)
					::SetWindowLongPtr(hWnd, GWL_STYLE, nStyle & ~WS_DISABLED);

				RECT rcClient{};
				::GetClientRect(hWnd, &rcClient);
				::MapWindowPoints(hWnd, ::GetParent(hWnd), reinterpret_cast<LPPOINT>(&rcClient), 2);
				::RedrawWindow(::GetParent(hWnd), &rcClient, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
				
				if (!pStaticTextData->isEnabled)
					::SetWindowLongPtr(hWnd, GWL_STYLE, nStyle | WS_DISABLED);

				return 0;
			}
		}
		return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	void subclassStaticText(HWND hWnd)
	{
		if (::GetWindowSubclass(hWnd, StaticTextSubclass, g_staticTextSubclassID, nullptr) == FALSE)
		{
			auto pStaticTextData = reinterpret_cast<DWORD_PTR>(new StaticTextData(hWnd));
			::SetWindowSubclass(hWnd, StaticTextSubclass, g_staticTextSubclassID, pStaticTextData);
		}
	}

	void subclassStaticText(HWND hWnd, DarkModeParams p)
	{
		if (p._subclass)
		{
			DarkMode::subclassStaticText(hWnd);
		}
	}

	void autoSubclassAndThemeChildControls(HWND hwndParent, bool subclass, bool theme)
	{
		DarkModeParams p{
			DarkMode::isExperimentalActive() ? L"DarkMode_Explorer" : nullptr
			, subclass
			, theme
		};

		::EnableThemeDialogTexture(hwndParent, theme && !DarkMode::isEnabled() ? ETDT_ENABLETAB : ETDT_DISABLE);

		::EnumChildWindows(hwndParent, [](HWND hWnd, LPARAM lParam) WINAPI_LAMBDA {
			const auto& p = *reinterpret_cast<DarkModeParams*>(lParam);
			std::wstring className = getWndClassName(hWnd);

			if (className == WC_BUTTON)
			{
				DarkMode::subclassAndThemeButton(hWnd, p);
				return TRUE;
			}

			if (className == WC_STATIC)
			{
				DarkMode::subclassStaticText(hWnd, p);
				return TRUE;
			}

			if (className == WC_COMBOBOX)
			{
				DarkMode::subclassAndThemeComboBox(hWnd, p);
				return TRUE;
			}

			if (className == WC_EDIT)
			{
				DarkMode::subclassAndThemeListBoxOrEditControl(hWnd, p, false);
				return TRUE;
			}

			if (className == WC_LISTBOX)
			{
				DarkMode::subclassAndThemeListBoxOrEditControl(hWnd, p, true);
				return TRUE;
			}

			if (className == WC_LISTVIEW)
			{
				DarkMode::subclassAndThemeListView(hWnd, p);
				return TRUE;
			}

			if (className == WC_TREEVIEW)
			{
				DarkMode::themeTreeView(hWnd, p);
				return TRUE;
			}

			if (className == TOOLBARCLASSNAME)
			{
				DarkMode::themeToolbar(hWnd, p);
				return TRUE;
			}

			if (className == UPDOWN_CLASS)
			{
				DarkMode::subclassAndThemeUpDownControl(hWnd, p);
				return TRUE;
			}

			if (className == WC_TABCONTROL)
			{
				DarkMode::subclassTabControl(hWnd, p);
				return TRUE;
			}

			if (className == STATUSCLASSNAME)
			{
				DarkMode::subclassStatusBarControl(hWnd, p);
				return TRUE;
			}

			if (className == WC_SCROLLBAR)
			{
				if (p._theme)
				{
					DarkMode::setDarkScrollBar(hWnd);
				}
				return TRUE;
			}

			if (className == WC_COMBOBOXEX)
			{
				DarkMode::subclassComboboxEx(hWnd, p);
				return TRUE;
			}

			if (className == PROGRESS_CLASS)
			{
				if (p._theme)
				{
					DarkMode::subclassProgressBarControl(hWnd, p);
				}
				else
				{
					DarkMode::themeProgressBar(hWnd, p);
				}
				return TRUE;
			}

			// Plugin might use rich edit control version 2.0 and later
			if (className == L"RichEdit20W" || className == L"RICHEDIT50W")
			{
				DarkMode::themeRichEdit(hWnd, p);
				return TRUE;
			}

			/*
			// for debugging 
			if (className == L"#32770")
			{
				return TRUE;
			}

			if (className == TRACKBAR_CLASS)
			{
				return TRUE;
			}
			*/

			return TRUE;
		}, reinterpret_cast<LPARAM>(&p));
	}

	void autoThemeChildControls(HWND hwndParent)
	{
		autoSubclassAndThemeChildControls(hwndParent, false, DarkMode::isWindows10());
	}

	void subclassAndThemeButton(HWND hWnd, DarkModeParams p)
	{
		const auto nButtonStyle = ::GetWindowLongPtr(hWnd, GWL_STYLE);
		switch (nButtonStyle & BS_TYPEMASK)
		{
			case BS_CHECKBOX:
			case BS_AUTOCHECKBOX:
			case BS_3STATE:
			case BS_AUTO3STATE:
			case BS_RADIOBUTTON:
			case BS_AUTORADIOBUTTON:
			{
				if ((nButtonStyle & BS_PUSHLIKE) == BS_PUSHLIKE)
				{
					if (p._theme)
					{
						::SetWindowTheme(hWnd, p._themeClassName, nullptr);
					}
					break;
				}

				if (DarkMode::isWindows11() && p._theme)
				{
					::SetWindowTheme(hWnd, p._themeClassName, nullptr);
				}

				if (p._subclass)
				{
					DarkMode::subclassButtonControl(hWnd);
				}
				break;
			}

			case BS_GROUPBOX:
			{
				if (p._subclass)
				{
					DarkMode::subclassGroupboxControl(hWnd);
				}
				break;
			}

			case BS_PUSHBUTTON:
			case BS_DEFPUSHBUTTON:
			case BS_SPLITBUTTON:
			case BS_DEFSPLITBUTTON:
			{
				if (p._theme)
				{
					::SetWindowTheme(hWnd, p._themeClassName, nullptr);
				}
				break;
			}

			default:
			{
				break;
			}
		}
	}

	void subclassAndThemeComboBox(HWND hWnd, DarkModeParams p)
	{
		const auto nStyle = ::GetWindowLongPtr(hWnd, GWL_STYLE);

		if ((nStyle & CBS_DROPDOWNLIST) == CBS_DROPDOWNLIST || (nStyle & CBS_DROPDOWN) == CBS_DROPDOWN)
		{
			COMBOBOXINFO cbi{};
			cbi.cbSize = sizeof(COMBOBOXINFO);
			if (::GetComboBoxInfo(hWnd, &cbi) == TRUE)
			{
				if (p._theme && cbi.hwndList)
				{
					//dark scrollbar for listbox of combobox
					::SetWindowTheme(cbi.hwndList, p._themeClassName, nullptr);
				}
			}

			if (p._subclass)
			{
				HWND hParent = ::GetParent(hWnd);
				if ((hParent == nullptr || getWndClassName(hParent) != WC_COMBOBOXEX))
				{
					DarkMode::subclassComboBoxControl(hWnd);
				}
			}

			if (p._theme && DarkMode::isExperimentalSupported())
			{
				DarkMode::allowDarkModeForWindow(hWnd, DarkMode::isExperimentalActive());
				::SetWindowTheme(hWnd, L"CFD", nullptr);
			}
		}
	}

	void subclassAndThemeListBoxOrEditControl(HWND hWnd, DarkModeParams p, bool isListBox)
	{
		const auto nStyle = ::GetWindowLongPtr(hWnd, GWL_STYLE);
		const bool hasScrollBar = ((nStyle & WS_HSCROLL) == WS_HSCROLL) || ((nStyle & WS_VSCROLL) == WS_VSCROLL);
		if (p._theme && (isListBox || hasScrollBar))
		{
			//dark scrollbar for listbox or edit control
			::SetWindowTheme(hWnd, p._themeClassName, nullptr);
		}

		const auto nExStyle = ::GetWindowLongPtr(hWnd, GWL_EXSTYLE);
		const bool hasClientEdge = (nExStyle & WS_EX_CLIENTEDGE) == WS_EX_CLIENTEDGE;
		const bool isCBoxListBox = isListBox && (nStyle & LBS_COMBOBOX) == LBS_COMBOBOX;

		if (p._subclass && hasClientEdge && !isCBoxListBox)
		{
			DarkMode::subclassCustomBorderForListBoxAndEditControls(hWnd);
		}

		bool changed = false;
		if (::GetWindowSubclass(hWnd, CustomBorderSubclass, g_customBorderSubclassID, nullptr) == TRUE)
		{
			if (DarkMode::isEnabled())
			{
				if (hasClientEdge)
				{
					::SetWindowLongPtr(hWnd, GWL_EXSTYLE, nExStyle & ~WS_EX_CLIENTEDGE);
					changed = true;
				}
			}
			else if (!hasClientEdge)
			{
				::SetWindowLongPtr(hWnd, GWL_EXSTYLE, nExStyle | WS_EX_CLIENTEDGE);
				changed = true;
			}
		}

		if (changed)
		{
			::SetWindowPos(hWnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
		}

	}

	void subclassAndThemeListView(HWND hWnd, DarkModeParams p)
	{
		if (p._theme)
		{
			DarkMode::setDarkListView(hWnd);
			DarkMode::setDarkTooltips(hWnd, DarkMode::ToolTipsType::listview);
		}

		ListView_SetTextColor(hWnd, DarkMode::getViewTextColor());
		ListView_SetTextBkColor(hWnd, DarkMode::getViewBackgroundColor());
		ListView_SetBkColor(hWnd, DarkMode::getViewBackgroundColor());

		if (p._subclass)
		{
			auto nExStyle = ListView_GetExtendedListViewStyle(hWnd);
			ListView_SetExtendedListViewStyle(hWnd, nExStyle | LVS_EX_DOUBLEBUFFER);
			DarkMode::subclassListViewControl(hWnd);
		}
	}

	void themeTreeView(HWND hWnd, DarkModeParams p)
	{
		TreeView_SetTextColor(hWnd, DarkMode::getViewTextColor());
		TreeView_SetBkColor(hWnd, DarkMode::getViewBackgroundColor());

		//DarkMode::calculateTreeViewStyle();
		DarkMode::setTreeViewStyle(hWnd, p._theme);

		if (p._theme)
		{
			DarkMode::setDarkTooltips(hWnd, DarkMode::ToolTipsType::treeview);
		}
	}

	void themeToolbar(HWND hWnd, DarkModeParams p)
	{
		DarkMode::setDarkLineAbovePanelToolbar(hWnd);

		if (p._theme)
		{
			DarkMode::setDarkTooltips(hWnd, DarkMode::ToolTipsType::toolbar);
		}
	}

	void themeRichEdit(HWND hWnd, DarkModeParams p)
	{
		if (p._theme)
		{
			//dark scrollbar for rich edit control
			::SetWindowTheme(hWnd, p._themeClassName, nullptr);
		}
	}

	void themeProgressBar(HWND hWnd, DarkModeParams p)
	{
		if (p._theme)
		{
			DarkMode::setBorder(hWnd, DarkMode::isEnabled(), WS_DLGFRAME);
			DarkMode::disableVisualStyle(hWnd, DarkMode::isEnabled());
			if (DarkMode::isEnabled())
			{
				::SendMessage(hWnd, PBM_SETBKCOLOR, 0, static_cast<LPARAM>(DarkMode::getBackgroundColor()));
				::SendMessage(hWnd, PBM_SETBARCOLOR, 0, static_cast<LPARAM>(HEXRGB(0x06B025)));
			}
		}
	}

	static LRESULT darkToolBarNotifyCustomDraw(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool isPlugin)
	{
		auto lptbcd = reinterpret_cast<LPNMTBCUSTOMDRAW>(lParam);
		static int roundCornerValue = 0;

		switch (lptbcd->nmcd.dwDrawStage)
		{
			case CDDS_PREPAINT:
			{
				LRESULT lr = CDRF_DODEFAULT;
				if (DarkMode::isEnabled())
				{
					if (DarkMode::isWindows11())
					{
						roundCornerValue = 5;
					}

					::FillRect(lptbcd->nmcd.hdc, &lptbcd->nmcd.rc, DarkMode::getDlgBackgroundBrush());
					lr |= CDRF_NOTIFYITEMDRAW;
				}

				if (isPlugin)
				{
					lr |= ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
				}

				return lr;
			}

			case CDDS_ITEMPREPAINT:
			{
				lptbcd->hbrMonoDither = DarkMode::getBackgroundBrush();
				lptbcd->hbrLines = DarkMode::getEdgeBrush();
				lptbcd->hpenLines = DarkMode::getEdgePen();
				lptbcd->clrText = DarkMode::getDarkerTextColor();
				lptbcd->clrTextHighlight = DarkMode::getTextColor();
				lptbcd->clrBtnFace = DarkMode::getBackgroundColor();
				lptbcd->clrBtnHighlight = DarkMode::getControlBackgroundColor();
				lptbcd->clrHighlightHotTrack = DarkMode::getHotBackgroundColor();
				lptbcd->nStringBkMode = TRANSPARENT;
				lptbcd->nHLStringBkMode = TRANSPARENT;

				if ((lptbcd->nmcd.uItemState & CDIS_HOT) == CDIS_HOT)
				{
					auto holdBrush = ::SelectObject(lptbcd->nmcd.hdc, DarkMode::getHotBackgroundBrush());
					auto holdPen = ::SelectObject(lptbcd->nmcd.hdc, DarkMode::getHotEdgePen());
					::RoundRect(lptbcd->nmcd.hdc, lptbcd->nmcd.rc.left, lptbcd->nmcd.rc.top, lptbcd->nmcd.rc.right, lptbcd->nmcd.rc.bottom, roundCornerValue, roundCornerValue);
					::SelectObject(lptbcd->nmcd.hdc, holdBrush);
					::SelectObject(lptbcd->nmcd.hdc, holdPen);

					lptbcd->nmcd.uItemState &= ~(CDIS_CHECKED | CDIS_HOT);
				}
				else if ((lptbcd->nmcd.uItemState & CDIS_CHECKED) == CDIS_CHECKED)
				{
					auto holdBrush = ::SelectObject(lptbcd->nmcd.hdc, DarkMode::getControlBackgroundBrush());
					auto holdPen = ::SelectObject(lptbcd->nmcd.hdc, DarkMode::getEdgePen());
					::RoundRect(lptbcd->nmcd.hdc, lptbcd->nmcd.rc.left, lptbcd->nmcd.rc.top, lptbcd->nmcd.rc.right, lptbcd->nmcd.rc.bottom, roundCornerValue, roundCornerValue);
					::SelectObject(lptbcd->nmcd.hdc, holdBrush);
					::SelectObject(lptbcd->nmcd.hdc, holdPen);

					lptbcd->nmcd.uItemState &= ~CDIS_CHECKED;
				}

				LRESULT lr = TBCDRF_USECDCOLORS;
				if ((lptbcd->nmcd.uItemState & CDIS_SELECTED) == CDIS_SELECTED)
				{
					lr |= TBCDRF_NOBACKGROUND;
				}

				if (isPlugin)
				{
					lr |= ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
				}

				return lr;
			}

			default:
				break;
		}
		return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	static void drawListviewItem(LPNMLVCUSTOMDRAW& lplvcd, bool isReport, bool hasGridlines)
	{
		HWND& hList = lplvcd->nmcd.hdr.hwndFrom;
		const auto isSelected = ListView_GetItemState(hList, lplvcd->nmcd.dwItemSpec, LVIS_SELECTED) == LVIS_SELECTED;
		const bool isHot = (lplvcd->nmcd.uItemState & CDIS_HOT) == CDIS_HOT;

		if (DarkMode::isEnabled())
		{
			HBRUSH hBrush = nullptr;

			if (isSelected)
			{
				lplvcd->clrText = DarkMode::getTextColor();
				lplvcd->clrTextBk = DarkMode::getControlBackgroundColor();
				hBrush = DarkMode::getControlBackgroundBrush();
			}
			else if (isHot)
			{
				lplvcd->clrText = DarkMode::getTextColor();
				lplvcd->clrTextBk = DarkMode::getHotBackgroundColor();
				hBrush = DarkMode::getHotBackgroundBrush();
			}

			if (hBrush != nullptr)
			{
				if (!isReport || (isReport && hasGridlines))
				{
					::FillRect(lplvcd->nmcd.hdc, &lplvcd->nmcd.rc, hBrush);
				}
				else
				{
					const auto hHeader = ListView_GetHeader(hList);
					const auto nCol = Header_GetItemCount(hHeader);
					const LONG paddingLeft = DarkMode::isThemeDark() ? 1 : 0;
					const LONG paddingRight = DarkMode::isThemeDark() ? 2 : 1;

					LVITEMINDEX lvii{ static_cast<int>(lplvcd->nmcd.dwItemSpec), 0 };
					RECT rcSubitem{
						lplvcd->nmcd.rc.left
						, lplvcd->nmcd.rc.top
						, lplvcd->nmcd.rc.left + ListView_GetColumnWidth(hList, 0) - paddingRight
						, lplvcd->nmcd.rc.bottom
					};
					::FillRect(lplvcd->nmcd.hdc, &rcSubitem, hBrush);

					for (int i = 1; i < nCol; ++i)
					{
						ListView_GetItemIndexRect(hList, &lvii, i, LVIR_BOUNDS, &rcSubitem);
						rcSubitem.left -= paddingLeft;
						rcSubitem.right -= paddingRight;
						::FillRect(lplvcd->nmcd.hdc, &rcSubitem, hBrush);
					}
				}
			}
			else if (hasGridlines)
			{
				::FillRect(lplvcd->nmcd.hdc, &lplvcd->nmcd.rc, DarkMode::getViewBackgroundBrush());
			}
		}

		if (isSelected)
		{
			::DrawFocusRect(lplvcd->nmcd.hdc, &lplvcd->nmcd.rc);
		}
		else if (isHot && !hasGridlines)
		{
			::FrameRect(lplvcd->nmcd.hdc, &lplvcd->nmcd.rc, DarkMode::isEnabled() ? DarkMode::getHotEdgeBrush() : ::GetSysColorBrush(COLOR_WINDOWTEXT));
		}
	}

	static LRESULT darkListViewNotifyCustomDraw(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool isPlugin)
	{
		auto lplvcd = reinterpret_cast<LPNMLVCUSTOMDRAW>(lParam);
		HWND& hList = lplvcd->nmcd.hdr.hwndFrom;
		const auto lvStyle = ::GetWindowLongPtr(hList, GWL_STYLE) & LVS_TYPEMASK;
		const bool isReport = (lvStyle == LVS_REPORT);
		bool hasGridlines = false;
		if (isReport)
		{
			const auto lvExStyle = ListView_GetExtendedListViewStyle(hList);
			hasGridlines = (lvExStyle & LVS_EX_GRIDLINES) == LVS_EX_GRIDLINES;
		}

		switch (lplvcd->nmcd.dwDrawStage)
		{
			case CDDS_PREPAINT:
			{
				LRESULT lr = CDRF_NOTIFYITEMDRAW;
				if (isReport && hasGridlines)
				{
					::FillRect(lplvcd->nmcd.hdc, &lplvcd->nmcd.rc, DarkMode::getViewBackgroundBrush());
				}

				if (isPlugin)
				{
					lr |= ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
				}

				return lr;
			}

			case CDDS_ITEMPREPAINT:
			{
				DarkMode::drawListviewItem(lplvcd, isReport, hasGridlines);

				LRESULT lr = CDRF_NEWFONT;

				if (isPlugin)
				{
					lr |= ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
				}

				return lr;
			}

			default:
				break;
		}
		return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	static LRESULT darkTreeViewNotifyCustomDraw(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool isPlugin)
	{
		auto lptvcd = reinterpret_cast<LPNMTVCUSTOMDRAW>(lParam);

		switch (lptvcd->nmcd.dwDrawStage)
		{
			case CDDS_PREPAINT:
			{
				LRESULT lr = DarkMode::isEnabled() ? CDRF_NOTIFYITEMDRAW : CDRF_DODEFAULT;

				if (isPlugin)
				{
					lr |= ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
				}

				return lr;
			}

			case CDDS_ITEMPREPAINT:
			{
				LRESULT lr = CDRF_DODEFAULT;

				if (DarkMode::isEnabled())
				{
					if ((lptvcd->nmcd.uItemState & CDIS_SELECTED) == CDIS_SELECTED)
					{
						lptvcd->clrText = DarkMode::getTextColor();
						lptvcd->clrTextBk = DarkMode::getControlBackgroundColor();
						::FillRect(lptvcd->nmcd.hdc, &lptvcd->nmcd.rc, DarkMode::getControlBackgroundBrush());

						lr |= CDRF_NEWFONT | CDRF_NOTIFYPOSTPAINT;
					}
					else if ((lptvcd->nmcd.uItemState & CDIS_HOT) == CDIS_HOT)
					{
						lptvcd->clrText = DarkMode::getTextColor();
						lptvcd->clrTextBk = DarkMode::getHotBackgroundColor();

						if (DarkMode::isWindows10() || g_treeViewStyle == TreeViewStyle::light)
						{
							::FillRect(lptvcd->nmcd.hdc, &lptvcd->nmcd.rc, DarkMode::getHotBackgroundBrush());
							lr |= CDRF_NOTIFYPOSTPAINT;
						}
						lr |= CDRF_NEWFONT;
					}
				}

				if (isPlugin)
				{
					lr |= ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
				}

				return lr;
			}

			case CDDS_ITEMPOSTPAINT:
			{
				if (DarkMode::isEnabled())
				{
					RECT rcFrame = lptvcd->nmcd.rc;
					rcFrame.left -= 1;
					rcFrame.right += 1;

					if ((lptvcd->nmcd.uItemState & CDIS_HOT) == CDIS_HOT)
					{
						DarkMode::paintRoundFrameRect(lptvcd->nmcd.hdc, rcFrame, DarkMode::getHotEdgePen(), 0, 0);
					}
					else if ((lptvcd->nmcd.uItemState & CDIS_SELECTED) == CDIS_SELECTED)
					{
						DarkMode::paintRoundFrameRect(lptvcd->nmcd.hdc, rcFrame, DarkMode::getEdgePen(), 0, 0);
					}
				}

				if (isPlugin)
				{
					break;
				}

				return CDRF_DODEFAULT;
			}

			default:
				break;
		}
		return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	static LRESULT darkTrackBarNotifyCustomDraw(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool isPlugin)
	{
		LPNMCUSTOMDRAW lpnmcd = reinterpret_cast<LPNMCUSTOMDRAW>(lParam);

		switch (lpnmcd->dwDrawStage)
		{
			case CDDS_PREPAINT:
			{
				LRESULT lr = DarkMode::isEnabled() ? CDRF_NOTIFYITEMDRAW : CDRF_DODEFAULT;

					if (isPlugin)
					{
						lr |= ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
					}

					return lr;
			}

			case CDDS_ITEMPREPAINT:
			{
				switch (lpnmcd->dwItemSpec)
				{
					case TBCD_THUMB:
					{
						if ((lpnmcd->uItemState & CDIS_SELECTED) == CDIS_SELECTED)
						{
							::FillRect(lpnmcd->hdc, &lpnmcd->rc, DarkMode::getControlBackgroundBrush());
							LRESULT lr = CDRF_SKIPDEFAULT;
							if (isPlugin)
							{
								lr |= ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
							}
							return lr;
						}
						break;
					}

					case TBCD_CHANNEL:
					{
						if (::IsWindowEnabled(lpnmcd->hdr.hwndFrom) == FALSE)
						{
							::FillRect(lpnmcd->hdc, &lpnmcd->rc, DarkMode::getDlgBackgroundBrush());
							DarkMode::paintRoundFrameRect(lpnmcd->hdc, lpnmcd->rc, DarkMode::getEdgePen(), 0, 0);
						}
						else
						{
							::FillRect(lpnmcd->hdc, &lpnmcd->rc, DarkMode::getControlBackgroundBrush());
						}

						LRESULT lr = CDRF_SKIPDEFAULT;
						if (isPlugin)
						{
							lr |= ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
						}
						return lr;
					}

					default:
						break;
				}
				break;
			}

			default:
				break;
			}
		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	constexpr UINT_PTR g_WindowCtlColorSubclassID = 42;

	static LRESULT CALLBACK WindowCtlColorSubclass(
		HWND hWnd,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam,
		UINT_PTR uIdSubclass,
		DWORD_PTR /*dwRefData*/
	)
	{
		switch (uMsg)
		{
			case WM_NCDESTROY:
			{
				::RemoveWindowSubclass(hWnd, WindowCtlColorSubclass, uIdSubclass);
				break;
			}

			case WM_ERASEBKGND:
			{
				if (DarkMode::isEnabled())
				{
					RECT rcClient{};
					::GetClientRect(hWnd, &rcClient);
					::FillRect(reinterpret_cast<HDC>(wParam), &rcClient, DarkMode::getDlgBackgroundBrush());
					return TRUE;
				}
				break;
			}

			case WM_CTLCOLOREDIT:
			{
				if (DarkMode::isEnabled())
				{
					return DarkMode::onCtlColorControl(reinterpret_cast<HDC>(wParam));
				}
				break;
			}

			case WM_CTLCOLORLISTBOX:
			{
				if (DarkMode::isEnabled())
				{
					return DarkMode::onCtlColorListbox(wParam, lParam);
				}
				break;
			}

			case WM_CTLCOLORDLG:
			{

				if (DarkMode::isEnabled())
				{
					return DarkMode::onCtlColorDlg(reinterpret_cast<HDC>(wParam));
				}
				break;
			}

			case WM_CTLCOLORSTATIC:
			{
				if (DarkMode::isEnabled())
				{
					auto hWndChild = reinterpret_cast<HWND>(lParam);
					auto hdc = reinterpret_cast<HDC>(wParam);
					if (cmpWndClassName(hWndChild, WC_EDIT))
					{
						if (::IsWindowEnabled(hWndChild) == TRUE)
						{
							return DarkMode::onCtlColor(hdc);
						}
						return DarkMode::onCtlColorDlg(hdc);
					}

					DWORD_PTR dwRefData = 0;
					if (::GetWindowSubclass(hWndChild, StaticTextSubclass, g_staticTextSubclassID, &dwRefData) == TRUE)
					{
						auto pStaticTextData = reinterpret_cast<StaticTextData*>(dwRefData);
						return DarkMode::onCtlColorDlgStaticText(hdc, pStaticTextData->isEnabled);
					}
					return DarkMode::onCtlColorDlg(hdc);
				}
				break;
			}

			case WM_PRINTCLIENT:
			{
				if (DarkMode::isEnabled())
				{
					return TRUE;
				}
				break;
			}
		}
		return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	void autoSubclassCtlColor(HWND hWnd)
	{
		if (::GetWindowSubclass(hWnd, WindowCtlColorSubclass, g_WindowCtlColorSubclassID, nullptr) == FALSE)
		{
			::SetWindowSubclass(hWnd, WindowCtlColorSubclass, g_WindowCtlColorSubclassID, 0);
		}
	}

	constexpr UINT_PTR g_windowNotifySubclassID = 42;

	static LRESULT CALLBACK WindowNotifySubclass(
		HWND hWnd,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam,
		UINT_PTR uIdSubclass,
		DWORD_PTR /*dwRefData*/
	)
	{
		switch (uMsg)
		{
			case WM_NCDESTROY:
			{
				::RemoveWindowSubclass(hWnd, WindowNotifySubclass, uIdSubclass);
				break;
			}

			case WM_NOTIFY:
			{
				auto nmhdr = reinterpret_cast<LPNMHDR>(lParam);
				std::wstring className = getWndClassName(nmhdr->hwndFrom);

				switch (nmhdr->code)
				{
					case NM_CUSTOMDRAW:
					{
						if (className == TOOLBARCLASSNAME)
						{
							return DarkMode::darkToolBarNotifyCustomDraw(hWnd, uMsg, wParam, lParam, false);
						}

						if (className == WC_LISTVIEW)
						{
							return DarkMode::darkListViewNotifyCustomDraw(hWnd, uMsg, wParam, lParam, false);
						}

						if (className == WC_TREEVIEW)
						{
							return DarkMode::darkTreeViewNotifyCustomDraw(hWnd, uMsg, wParam, lParam, false);
						}

						if (className == TRACKBAR_CLASS)
						{
							return DarkMode::darkTrackBarNotifyCustomDraw(hWnd, uMsg, wParam, lParam, false);
						}
					}
					break;
				}
				break;
			}
		}
		return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	void autoSubclassNotifyCustomDraw(HWND hWnd, bool subclassChildren)
	{
		if (::GetWindowSubclass(hWnd, WindowNotifySubclass, g_windowNotifySubclassID, nullptr) == FALSE)
		{
			::SetWindowSubclass(hWnd, WindowNotifySubclass, g_windowNotifySubclassID, 0);
			if (subclassChildren)
			{
				DarkMode::autoSubclassAndThemeChildControls(hWnd);
				if (g_enableWindowsMode)
					DarkMode::autoSubclassWindowSettingChange(hWnd);
			}
		}
	}

	constexpr UINT_PTR g_windowMenuBarSubclassID = 42;

	static LRESULT CALLBACK WindowMenuBarSubclass(
		HWND hWnd,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam,
		UINT_PTR uIdSubclass,
		DWORD_PTR /*dwRefData*/
	)
	{
		LRESULT result = FALSE;

		if (DarkMode::isEnabled() && DarkMode::runUAHWndProc(hWnd, uMsg, wParam, lParam, &result))
		{
			return result;
		}

		switch (uMsg)
		{
			case WM_NCDESTROY:
			{
				::RemoveWindowSubclass(hWnd, WindowMenuBarSubclass, uIdSubclass);
				break;
			}

			case WM_NCACTIVATE:
			{
				result = ::DefWindowProc(hWnd, uMsg, wParam, lParam);
				DarkMode::drawUAHMenuNCBottomLine(hWnd);
				return result;
			}

			case WM_NCPAINT:
			{
				result = ::DefWindowProc(hWnd, uMsg, wParam, lParam);
				DarkMode::drawUAHMenuNCBottomLine(hWnd);
				return result;
			}
		}
		return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	void autoSubclassWindowMenuBar(HWND hWnd)
	{
		if (::GetWindowSubclass(hWnd, WindowMenuBarSubclass, g_windowMenuBarSubclassID, nullptr) == FALSE)
		{
			::SetWindowSubclass(hWnd, WindowMenuBarSubclass, g_windowMenuBarSubclassID, 0);
		}
	}

	constexpr UINT_PTR g_windowSettingChangeSubclassID = 42;

	static LRESULT CALLBACK WindowSettingChangeSubclass(
		HWND hWnd,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam,
		UINT_PTR uIdSubclass,
		DWORD_PTR /*dwRefData*/
	)
	{
		switch (uMsg)
		{
			case WM_NCDESTROY:
			{
				::RemoveWindowSubclass(hWnd, WindowSettingChangeSubclass, uIdSubclass);
				break;
			}

			case WM_SETTINGCHANGE:
			{
				if (DarkMode::handleSettingChange(lParam))
				{
					DarkMode::setDarkTitleBar(hWnd);
					DarkMode::autoThemeChildControls(hWnd);
					::RedrawWindow(hWnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
				}
				break;
			}
		}
		return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	void autoSubclassWindowSettingChange(HWND hWnd)
	{
		if (::GetWindowSubclass(hWnd, WindowSettingChangeSubclass, g_windowSettingChangeSubclassID, nullptr) == FALSE)
		{
			::SetWindowSubclass(hWnd, WindowSettingChangeSubclass, g_windowSettingChangeSubclassID, 0);
		}
	}

	void setDarkTitleBar(HWND hWnd)
	{
		constexpr DWORD win10Build2004 = 19041;
		constexpr DWORD win11Mica = 22621;
		if (DarkMode::getWindowsBuildNumber() >= win10Build2004)
		{
			BOOL value = DarkMode::isExperimentalActive() ? TRUE : FALSE;
			::DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));

			if (DarkMode::getWindowsBuildNumber() >= win11Mica)
			{
				if (g_micaExtend && g_mica != DWMSBT_AUTO && !g_enableWindowsMode && g_useDarkMode)
				{
					constexpr MARGINS margins = { -1 };
					::DwmExtendFrameIntoClientArea(hWnd, &margins);
				}

				::DwmSetWindowAttribute(hWnd, DWMWA_SYSTEMBACKDROP_TYPE, &g_mica, sizeof(g_mica));
			}
		}
		else
		{
			DarkMode::allowDarkModeForWindow(hWnd, DarkMode::isExperimentalActive());
			DarkMode::setTitleBarThemeColor(hWnd);
		}
	}

	void setDarkExplorerTheme(HWND hWnd)
	{
		::SetWindowTheme(hWnd, DarkMode::isExperimentalActive() ? L"DarkMode_Explorer" : nullptr, nullptr);
	}

	void setDarkScrollBar(HWND hWnd)
	{
		DarkMode::setDarkExplorerTheme(hWnd);
	}

	void setDarkTooltips(HWND hWnd, ToolTipsType type)
	{
		UINT msg = 0;
		switch (type)
		{
			case DarkMode::ToolTipsType::toolbar:
				msg = TB_GETTOOLTIPS;
				break;
			case DarkMode::ToolTipsType::listview:
				msg = LVM_GETTOOLTIPS;
				break;
			case DarkMode::ToolTipsType::treeview:
				msg = TVM_GETTOOLTIPS;
				break;
			case DarkMode::ToolTipsType::tabbar:
				msg = TCM_GETTOOLTIPS;
				break;
			case DarkMode::ToolTipsType::tooltip:
				msg = 0;
				break;
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

	void setDarkListView(HWND hWnd)
	{
		if (DarkMode::isExperimentalSupported())
		{
			const bool useDark = DarkMode::isExperimentalActive();

			HWND hHeader = ListView_GetHeader(hWnd);
			//DarkMode::allowDarkModeForWindow(hHeader, useDark);
			//::SetWindowTheme(hHeader, useDark ? L"ItemsView" : nullptr, nullptr);
			DarkMode::subclassHeaderControl(hHeader);

			DarkMode::allowDarkModeForWindow(hWnd, useDark);
			::SetWindowTheme(hWnd, L"Explorer", nullptr);
		}
	}

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

	// range to determine when it should be better to use classic style
	constexpr double g_middleGrayRange = 2.0;

	void calculateTreeViewStyle()
	{
		COLORREF bgColor = DarkMode::getViewBackgroundColor();

		if (g_treeViewBg != bgColor || g_lightnessTreeView == 50.0)
		{
			g_lightnessTreeView = calculatePerceivedLightness(bgColor);
			g_treeViewBg = bgColor;
		}

		if (g_lightnessTreeView < (50.0 - g_middleGrayRange))
		{
			g_treeViewStyle = TreeViewStyle::dark;
		}
		else if (g_lightnessTreeView > (50.0 + g_middleGrayRange))
		{
			g_treeViewStyle = TreeViewStyle::light;
		}
		else
		{
			g_treeViewStyle = TreeViewStyle::classic;
		}
	}

	void updatePrevTreeViewStyle()
	{
		g_treeViewStylePrev = g_treeViewStyle;
	}

	TreeViewStyle getTreeViewStyle()
	{
		const auto style = g_treeViewStyle;
		return style;
	}

	void setTreeViewStyle(HWND hWnd, bool force)
	{
		if (force || g_treeViewStylePrev != g_treeViewStyle)
		{
			auto nStyle = ::GetWindowLongPtr(hWnd, GWL_STYLE);
			const bool hasHotStyle = (nStyle & TVS_TRACKSELECT) == TVS_TRACKSELECT;
			bool change = false;
			std::wstring strSubAppName;

			switch (g_treeViewStyle)
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

	bool isThemeDark()
	{
		return g_treeViewStyle == TreeViewStyle::dark;
	}

	void setBorder(HWND hWnd, bool border, long borderStyle)
	{
		auto nStyle = ::GetWindowLongPtr(hWnd, GWL_STYLE);
		const bool hasBorder = (nStyle & borderStyle) == borderStyle;
		bool change = false;

		if (!hasBorder && border)
		{
			nStyle |= borderStyle;
			change = true;
		}
		else if (hasBorder && !border)
		{
			nStyle &= ~borderStyle;
			change = true;
		}

		if (change)
		{
			::SetWindowLongPtr(hWnd, GWL_STYLE, nStyle);
			::SetWindowPos(hWnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
		}
	}

	LRESULT onCtlColor(HDC hdc)
	{
		if (!DarkMode::isEnabled())
		{
			return FALSE;
		}

		::SetTextColor(hdc, DarkMode::getTextColor());
		::SetBkColor(hdc, DarkMode::getBackgroundColor());
		return reinterpret_cast<LRESULT>(DarkMode::getBackgroundBrush());
	}

	LRESULT onCtlColorControl(HDC hdc)
	{
		if (!DarkMode::isEnabled())
		{
			return FALSE;
		}

		::SetTextColor(hdc, DarkMode::getTextColor());
		::SetBkColor(hdc, DarkMode::getControlBackgroundColor());
		return reinterpret_cast<LRESULT>(DarkMode::getControlBackgroundBrush());
	}

	LRESULT onCtlColorDlg(HDC hdc)
	{
		if (!DarkMode::isEnabled())
		{
			return FALSE;
		}

		::SetTextColor(hdc, DarkMode::getTextColor());
		::SetBkColor(hdc, DarkMode::getDlgBackgroundColor());
		return reinterpret_cast<LRESULT>(DarkMode::getDlgBackgroundBrush());
	}

	LRESULT onCtlColorError(HDC hdc)
	{
		if (!DarkMode::isEnabled())
		{
			return FALSE;
		}

		::SetTextColor(hdc, DarkMode::getTextColor());
		::SetBkColor(hdc, DarkMode::getErrorBackgroundColor());
		return reinterpret_cast<LRESULT>(DarkMode::getErrorBackgroundBrush());
	}
	
	LRESULT onCtlColorDlgStaticText(HDC hdc, bool isTextEnabled)
	{
		if (!DarkMode::isEnabled())
		{
			::SetTextColor(hdc, ::GetSysColor(isTextEnabled ? COLOR_WINDOWTEXT : COLOR_GRAYTEXT));
			return FALSE;
		}

		::SetTextColor(hdc, isTextEnabled ? DarkMode::getTextColor() : DarkMode::getDisabledTextColor());
		::SetBkColor(hdc, DarkMode::getDlgBackgroundColor());
		return reinterpret_cast<LRESULT>(DarkMode::getDlgBackgroundBrush());
	}

	LRESULT onCtlColorListbox(WPARAM wParam, LPARAM lParam)
	{
		auto hdc = reinterpret_cast<HDC>(wParam);
		auto hWnd = reinterpret_cast<HWND>(lParam);

		const auto nStyle = ::GetWindowLongPtr(hWnd, GWL_STYLE);
		const bool isComboBox = (nStyle & LBS_COMBOBOX) == LBS_COMBOBOX;
		if ((!isComboBox || !DarkMode::isExperimentalActive()) && ::IsWindowEnabled(hWnd))
		{
			return DarkMode::onCtlColorControl(hdc);
		}
		return DarkMode::onCtlColor(hdc);
	}
}
