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

	struct DarkModeParams
	{
		const wchar_t* _themeClassName = nullptr;
		bool _subclass = false;
		bool _theme = false;
	};

	enum class ToolTipsType
	{
		tooltip,
		toolbar,
		listview,
		treeview,
		tabbar
	};

	enum ColorTone {
		blackTone  = 0,
		redTone    = 1,
		greenTone  = 2,
		blueTone   = 3,
		purpleTone = 4,
		cyanTone   = 5,
		oliveTone  = 6,
		customizedTone = 32
	};

	enum class TreeViewStyle
	{
		classic = 0,
		light = 1,
		dark = 2
	};

	void initDarkMode();

	bool isEnabled();
	bool isExperimentalActive();
	bool isExperimentalSupported();

	bool isWindowsModeEnabled();

	bool isWindows10();
	bool isWindows11();
	DWORD getWindowsBuildNumber();

	double calculatePerceivedLightness(COLORREF c);

	void setDarkCustomColors(ColorTone colorTone);

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

	void changeCustomTheme(const Colors& colors);

	COLORREF getViewBackgroundColor();
	COLORREF getViewTextColor();
	COLORREF getViewGridlinesColor();
	HBRUSH getViewBackgroundBrush();
	HBRUSH getViewGridlinesBrush();

	// handle events
	bool handleSettingChange(LPARAM lParam);
	bool isDarkModeReg();

	// from DarkMode.h
	void initExperimentalDarkMode();
	void setDarkMode(bool useDark, bool fixDarkScrollbar);
	void allowDarkModeForApp(bool allow);
	bool allowDarkModeForWindow(HWND hWnd, bool allow);
	void setTitleBarThemeColor(HWND hWnd);

	void setSysColor(int nIndex, COLORREF color);
	bool hookSysColor();
	void unhookSysColor();

	// enhancements to DarkMode.h
	void enableDarkScrollBarForWindowAndChildren(HWND hWnd);

	void paintRoundRect(HDC hdc, const RECT rect, const HPEN hpen, const HBRUSH hBrush, int width = 0, int height = 0);
	inline void paintRoundFrameRect(HDC hdc, const RECT rect, const HPEN hpen, int width = 0, int height = 0);

	void subclassButtonControl(HWND hWnd);
	void subclassGroupboxControl(HWND hWnd);
	void subclassTabControl(HWND hWnd);
	void subclassComboBoxControl(HWND hWnd);
	void subclassListViewControl(HWND hWnd);
	void subclassStatusBarControl(HWND hWnd);
	void subclassProgressBarControl(HWND hWnd);
	bool subclassTabUpDownControl(HWND hWnd);

	void subclassAndThemeButton(HWND hWnd, DarkModeParams p);
	void subclassAndThemeComboBox(HWND hWnd, DarkModeParams p);
	void subclassAndThemeListBoxOrEditControl(HWND hWnd, DarkModeParams p, bool isListBox);
	void subclassAndThemeListView(HWND hWnd, DarkModeParams p);
	void themeTreeView(HWND hWnd, DarkModeParams p);
	void themeToolbar(HWND hWnd, DarkModeParams p);
	void themeRichEdit(HWND hWnd, DarkModeParams p);
	void themeProgressBar(HWND hWnd, DarkModeParams p);
	void subclassTabControl(HWND hWnd, DarkModeParams p);
	void subclassStatusBarControl(HWND hWnd, DarkModeParams p);
	void subclassProgressBarControl(HWND hWnd, DarkModeParams p);
	void subclassComboboxEx(HWND hWnd, DarkModeParams p);

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

	void disableVisualStyle(HWND hWnd, bool doDisable);
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
