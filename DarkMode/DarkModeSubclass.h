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

	void initDarkMode();

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

	void setDarkCustomColors(ColorTone colorTone);

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

	void paintRoundRect(HDC hdc, const RECT rect, const HPEN hpen, const HBRUSH hBrush, int width = 0, int height = 0);
	inline void paintRoundFrameRect(HDC hdc, const RECT rect, const HPEN hpen, int width = 0, int height = 0);

	void subclassButtonControl(HWND hWnd);
	void subclassGroupboxControl(HWND hWnd);
	bool subclassUpDownControl(HWND hWnd);
	void subclassTabControlUpDown(HWND hWnd);
	void subclassTabControl(HWND hWnd);
	void subclassComboBoxControl(HWND hWnd);
	void subclassComboboxExControl(HWND hWnd);
	void subclassListViewControl(HWND hWnd);
	void subclassHeaderControl(HWND hWnd);
	void subclassStatusBarControl(HWND hWnd);
	void subclassProgressBarControl(HWND hWnd);
	void subclassStaticText(HWND hWnd);

	void autoSubclassAndThemeChildControls(HWND hWndParent, bool subclass = true, bool theme = true);
	void autoThemeChildControls(HWND hWndParent);

	void autoSubclassCtlColor(HWND hWnd);
	void autoSubclassNotifyCustomDraw(HWND hWnd, bool subclassChildren = false);
	void autoSubclassWindowMenuBar(HWND hWnd);
	void autoSubclassWindowSettingChange(HWND hWnd);

	void setDarkTitleBar(HWND hWnd);
	void setDarkExplorerTheme(HWND hWnd);
	void setDarkScrollBar(HWND hWnd);
	void setDarkTooltips(HWND hWnd, ToolTipsType type = ToolTipsType::tooltip);
	void setDarkLineAbovePanelToolbar(HWND hWnd);
	void setDarkListView(HWND hWnd);
	void setDarkThemeExperimental(HWND hWnd, const wchar_t* themeClassName = L"Explorer");

	void disableVisualStyle(HWND hWnd, bool doDisable);
	double calculatePerceivedLightness(COLORREF clr);
	void calculateTreeViewStyle();
	void updatePrevTreeViewStyle();
	TreeViewStyle getTreeViewStyle();
	void setTreeViewStyle(HWND hWnd, bool force = false);
	bool isThemeDark();
	void setBorder(HWND hWnd, bool border = true, LONG_PTR borderStyle = WS_BORDER);

	LRESULT onCtlColor(HDC hdc);
	LRESULT onCtlColorCtrl(HDC hdc);
	LRESULT onCtlColorDlg(HDC hdc);
	LRESULT onCtlColorError(HDC hdc);
	LRESULT onCtlColorDlgStaticText(HDC hdc, bool isTextEnabled);
	LRESULT onCtlColorDlgLinkText(HDC hdc, bool isTextEnabled = true);
	LRESULT onCtlColorListbox(WPARAM wParam, LPARAM lParam);
}
