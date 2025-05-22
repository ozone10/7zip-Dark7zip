// Copyright (C)2024-2025 ozone10

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
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

// Based on Notepad++ dark mode code, original by adzm / Adam D. Walling
// with modification from Notepad++ team.
// Heavily modified by ozone10 (contributor of Notepad++)


#pragma once

#include <windows.h>

#if (NTDDI_VERSION >= NTDDI_VISTA) /* && \
	(defined(__x86_64__) || defined(_M_X64) || \
	 defined(__arm64__) || defined(__arm64) || defined(_M_ARM64))*/

namespace DarkMode
{
	struct Colors
	{
		COLORREF background = 0;
		COLORREF ctrlBackground = 0;
		COLORREF hotBackground = 0;
		COLORREF dlgBackground = 0;
		COLORREF errorBackground = 0;
		COLORREF text = 0;
		COLORREF darkerText = 0;
		COLORREF disabledText = 0;
		COLORREF linkText = 0;
		COLORREF edge = 0;
		COLORREF hotEdge = 0;
		COLORREF disabledEdge = 0;
	};

	struct ColorsView
	{
		COLORREF background = 0;
		COLORREF text = 0;
		COLORREF gridlines = 0;
		COLORREF headerBackground = 0;
		COLORREF headerHotBackground = 0;
		COLORREF headerText = 0;
		COLORREF headerEdge = 0;
	};

	enum class ToolTipsType
	{
		tooltip,
		toolbar,
		listview,
		treeview,
		tabbar
	};

	enum class ColorTone
	{
		black       = 0,
		red         = 1,
		green       = 2,
		blue        = 3,
		purple      = 4,
		cyan        = 5,
		olive       = 6,
		customized  = 32
	};

	enum class TreeViewStyle
	{
		classic = 0,
		light   = 1,
		dark    = 2
	};

	enum LibInfoType
	{
		featureCheck    = 0,
		verMajor        = 1,
		verMinor        = 2,
		verRevision     = 3,
		iathookExternal = 4,
		iniConfigUsed   = 5,
		allowOldOS      = 6,
		useDlgProcCtl   = 7,
		maxValue        = 8
	};

	int getLibInfo(LibInfoType libInfoType);

	// enum DarkModeType { light = 0, dark = 1, classic = 3 }; values
	void setDarkModeTypeConfig(UINT dmType);
	// DWM_WINDOW_CORNER_PREFERENCE values
	void setRoundCornerConfig(UINT roundCornerStyle);
	void setBorderColorConfig(COLORREF clr);
	// DWM_SYSTEMBACKDROP_TYPE values
	void setMicaConfig(UINT mica);
	void setMicaExtendedConfig(bool extendMica);

	void initDarkMode(const wchar_t* iniName);
	void initDarkMode();
	// enum DarkModeType { light = 0, dark = 1, classic = 3 }; values
	void setDarkMode(UINT dmType);

	bool isEnabled();
	bool isExperimentalActive();
	bool isExperimentalSupported();

	bool isWindowsModeEnabled();

	bool isWindows10();
	bool isWindows11();
	DWORD getWindowsBuildNumber();

	// handle events

	bool handleSettingChange(LPARAM lParam);
	bool isDarkModeReg();

	// from DarkMode.h

	void setSysColor(int nIndex, COLORREF color);
	bool hookSysColor();
	void unhookSysColor();

	// enhancements to DarkMode.h

	void enableDarkScrollBarForWindowAndChildren(HWND hWnd);

	// colors

	void setDarkCustomColors(ColorTone colorTone);
	ColorTone getColorTone();

	COLORREF setBackgroundColor(COLORREF clrNew);
	COLORREF setCtrlBackgroundColor(COLORREF clrNew);
	COLORREF setHotBackgroundColor(COLORREF clrNew);
	COLORREF setDlgBackgroundColor(COLORREF clrNew);
	COLORREF setErrorBackgroundColor(COLORREF clrNew);

	COLORREF setTextColor(COLORREF clrNew);
	COLORREF setDarkerTextColor(COLORREF clrNew);
	COLORREF setDisabledTextColor(COLORREF clrNew);
	COLORREF setLinkTextColor(COLORREF clrNew);

	COLORREF setEdgeColor(COLORREF clrNew);
	COLORREF setHotEdgeColor(COLORREF clrNew);
	COLORREF setDisabledEdgeColor(COLORREF clrNew);

	void changeCustomTheme(const Colors& colors);
	void updateBrushesAndPens();

	COLORREF getBackgroundColor();
	COLORREF getCtrlBackgroundColor();
	COLORREF getHotBackgroundColor();
	COLORREF getDlgBackgroundColor();
	COLORREF getErrorBackgroundColor();

	COLORREF getTextColor();
	COLORREF getDarkerTextColor();
	COLORREF getDisabledTextColor();
	COLORREF getLinkTextColor();

	COLORREF getEdgeColor();
	COLORREF getHotEdgeColor();
	COLORREF getDisabledEdgeColor();

	HBRUSH getBackgroundBrush();
	HBRUSH getDlgBackgroundBrush();
	HBRUSH getCtrlBackgroundBrush();
	HBRUSH getHotBackgroundBrush();
	HBRUSH getErrorBackgroundBrush();

	HBRUSH getEdgeBrush();
	HBRUSH getHotEdgeBrush();
	HBRUSH getDisabledEdgeBrush();

	HPEN getDarkerTextPen();
	HPEN getEdgePen();
	HPEN getHotEdgePen();
	HPEN getDisabledEdgePen();

	COLORREF setViewBackgroundColor(COLORREF clrNew);
	COLORREF setViewTextColor(COLORREF clrNew);
	COLORREF setViewGridlinesColor(COLORREF clrNew);

	COLORREF setHeaderBackgroundColor(COLORREF clrNew);
	COLORREF setHeaderHotBackgroundColor(COLORREF clrNew);
	COLORREF setHeaderTextColor(COLORREF clrNew);

	void updateBrushesAndPensView();

	COLORREF getViewBackgroundColor();
	COLORREF getViewTextColor();
	COLORREF getViewGridlinesColor();

	COLORREF getHeaderBackgroundColor();
	COLORREF getHeaderHotBackgroundColor();
	COLORREF getHeaderTextColor();

	HBRUSH getViewBackgroundBrush();
	HBRUSH getViewGridlinesBrush();

	HBRUSH getHeaderBackgroundBrush();
	HBRUSH getHeaderHotBackgroundBrush();

	HPEN getHeaderEdgePen();

	// paint helper

	void paintRoundRect(HDC hdc, const RECT& rect, HPEN hpen, HBRUSH hBrush, int width = 0, int height = 0);
	inline void paintRoundFrameRect(HDC hdc, const RECT& rect, HPEN hpen, int width = 0, int height = 0);

	// control subclassing

	void setCheckboxOrRadioBtnCtrlSubclass(HWND hWnd);
	void removeCheckboxOrRadioBtnCtrlSubclass(HWND hWnd);

	void setGroupboxCtrlSubclass(HWND hWnd);
	void removeGroupboxCtrlSubclass(HWND hWnd);

	void setUpDownCtrlSubclass(HWND hWnd);
	void removeUpDownCtrlSubclass(HWND hWnd);

	void setTabCtrlUpDownSubclass(HWND hWnd);
	void removeTabCtrlUpDownSubclass(HWND hWnd);
	void setTabCtrlSubclass(HWND hWnd);
	void removeTabCtrlSubclass(HWND hWnd);

	void setComboBoxCtrlSubclass(HWND hWnd);
	void removeComboBoxCtrlSubclass(HWND hWnd);

	void setComboBoxExCtrlSubclass(HWND hWnd);
	void removeComboBoxExCtrlSubclass(HWND hWnd);

	void setListViewCtrlSubclass(HWND hWnd);
	void removeListViewCtrlSubclass(HWND hWnd);

	void setHeaderCtrlSubclass(HWND hWnd);
	void removeHeaderCtrlSubclass(HWND hWnd);

	void setStatusBarCtrlSubclass(HWND hWnd);
	void removeStatusBarCtrlSubclass(HWND hWnd);

	void setProgressBarCtrlSubclass(HWND hWnd);
	void removeProgressBarCtrlSubclass(HWND hWnd);

	void setStaticTextCtrlSubclass(HWND hWnd);
	void removeStaticTextCtrlSubclass(HWND hWnd);

	// child subclassing

	void setChildCtrlsSubclassAndTheme(HWND hParent, bool subclass = true, bool theme = true);
	void setChildCtrlsTheme(HWND hParent);

	// window, parent, and other subclassing

	void setWindowEraseBgSubclass(HWND hWnd);
	void removeWindowEraseBgSubclass(HWND hWnd);

	void setWindowCtlColorSubclass(HWND hWnd);
	void removeWindowCtlColorSubclass(HWND hWnd);

	void setWindowNotifyCustomDrawSubclass(HWND hWnd, bool subclassChildren = false);
	void removeWindowNotifyCustomDrawSubclass(HWND hWnd);

	void setWindowMenuBarSubclass(HWND hWnd);
	void removeWindowMenuBarSubclass(HWND hWnd);

	void setWindowSettingChangeSubclass(HWND hWnd);
	void removeWindowSettingChangeSubclass(HWND hWnd);

	// theme and helper

	void setDarkTitleBarEx(HWND hWnd, bool useWin11Features);
	void setDarkTitleBar(HWND hWnd);
	void setDarkExplorerTheme(HWND hWnd);
	void setDarkScrollBar(HWND hWnd);
	void setDarkTooltips(HWND hWnd, ToolTipsType type = ToolTipsType::tooltip);
	void setDarkLineAbovePanelToolbar(HWND hWnd);
	void setDarkListView(HWND hWnd);
	void setDarkThemeExperimental(HWND hWnd, const wchar_t* themeClassName = L"Explorer");

	void setDarkDlgSafe(HWND hWnd, bool useWin11Features = true);
	void setDarkDlgNotifySafe(HWND hWnd, bool useWin11Features = true);

	// only if g_dmType == DarkModeType::classic
	inline void enableThemeDialogTexture(HWND hWnd, bool theme);
	void disableVisualStyle(HWND hWnd, bool doDisable);
	double calculatePerceivedLightness(COLORREF clr);
	void calculateTreeViewStyle();
	void updatePrevTreeViewStyle();
	TreeViewStyle getTreeViewStyle();
	void setTreeViewStyle(HWND hWnd, bool force = false);
	bool isThemeDark();
	inline void redrawWindowFrame(HWND hWnd);
	void setWindowStyle(HWND hWnd, bool setStyle, LONG_PTR styleFlag);
	void setWindowExStyle(HWND hWnd, bool setExStyle, LONG_PTR exStyleFlag);
	void setProgressBarClassicTheme(HWND hWnd);

	// ctl color

	LRESULT onCtlColor(HDC hdc);
	LRESULT onCtlColorCtrl(HDC hdc);
	LRESULT onCtlColorDlg(HDC hdc);
	LRESULT onCtlColorError(HDC hdc);
	LRESULT onCtlColorDlgStaticText(HDC hdc, bool isTextEnabled);
	LRESULT onCtlColorDlgLinkText(HDC hdc, bool isTextEnabled = true);
	LRESULT onCtlColorListbox(WPARAM wParam, LPARAM lParam);
} // namespace DarkMode

#else
#define _DARKMODELIB_NOT_USED
#endif // (NTDDI_VERSION >= NTDDI_VISTA) //&& (x64 or arm64)
