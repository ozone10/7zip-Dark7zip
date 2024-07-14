// Copyright (C)2024 ozone10

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
		COLORREF softerBackground = 0;
		COLORREF hotBackground = 0;
		COLORREF pureBackground = 0;
		COLORREF errorBackground = 0;
		COLORREF text = 0;
		COLORREF darkerText = 0;
		COLORREF disabledText = 0;
		COLORREF linkText = 0;
		COLORREF edge = 0;
		COLORREF hotEdge = 0;
		COLORREF disabledEdge = 0;
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

	bool isWindows10();
	bool isWindows11();
	DWORD getWindowsBuildNumber();

	double calculatePerceivedLightness(COLORREF c);

	COLORREF getBackgroundColor();
	COLORREF getSofterBackgroundColor();
	COLORREF getHotBackgroundColor();
	COLORREF getDarkerBackgroundColor();
	COLORREF getErrorBackgroundColor();

	COLORREF getTextColor();
	COLORREF getDarkerTextColor();
	COLORREF getDisabledTextColor();
	COLORREF getLinkTextColor();

	COLORREF getEdgeColor();
	COLORREF getHotEdgeColor();
	COLORREF getDisabledEdgeColor();

	HBRUSH getBackgroundBrush();
	HBRUSH getDarkerBackgroundBrush();
	HBRUSH getSofterBackgroundBrush();
	HBRUSH getHotBackgroundBrush();
	HBRUSH getErrorBackgroundBrush();

	HBRUSH getEdgeBrush();
	HBRUSH getHotEdgeBrush();
	HBRUSH getDisabledEdgeBrush();

	HPEN getDarkerTextPen();
	HPEN getEdgePen();
	HPEN getHotEdgePen();
	HPEN getDisabledEdgePen();

	// processes messages related to UAH / custom menubar drawing.
	// return true if handled, false to continue with normal processing in your wndproc
	bool runUAHWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* lr);
	void drawUAHMenuNCBottomLine(HWND hWnd);

	// from DarkMode.h
	void initExperimentalDarkMode();
	void setDarkMode(bool useDark, bool fixDarkScrollbar);
	void allowDarkModeForApp(bool allow);
	bool allowDarkModeForWindow(HWND hWnd, bool allow);
	void setTitleBarThemeColor(HWND hWnd);

	// enhancements to DarkMode.h
	void enableDarkScrollBarForWindowAndChildren(HWND hwnd);

	inline void paintRoundFrameRect(HDC hdc, const RECT rect, const HPEN hpen, int width = 0, int height = 0);

	void subclassButtonControl(HWND hwnd);
	void subclassGroupboxControl(HWND hwnd);
	void subclassTabControl(HWND hwnd);
	void subclassComboBoxControl(HWND hwnd);
	void subclassStatusBar(HWND hwnd);
	bool subclassTabUpDownControl(HWND hwnd);

	void subclassAndThemeButton(HWND hwnd, DarkModeParams p);
	void subclassAndThemeComboBox(HWND hwnd, DarkModeParams p);
	void subclassAndThemeListBoxOrEditControl(HWND hwnd, DarkModeParams p, bool isListBox);
	void subclassAndThemeListView(HWND hwnd, DarkModeParams p);
	void themeTreeView(HWND hwnd, DarkModeParams p);
	void themeToolbar(HWND hwnd, DarkModeParams p);
	void themeRichEdit(HWND hwnd, DarkModeParams p);
	void subclassTabControl(HWND hwnd, DarkModeParams p);
	void subclassStatusBar(HWND hwnd, DarkModeParams p);

	void autoSubclassAndThemeChildControls(HWND hwndParent, bool subclass = true, bool theme = true);
	void autoThemeChildControls(HWND hwndParent);

	void autoSubclassCtlColor(HWND hwnd);
	void autoSubclassNotifyCustomDraw(HWND hwnd, bool subclassChildren = false);
	void autoSubclassWindowMenuBar(HWND hwnd);

	void setDarkTitleBar(HWND hwnd);
	void setDarkExplorerTheme(HWND hwnd);
	void setDarkScrollBar(HWND hwnd);
	void setDarkTooltips(HWND hwnd, ToolTipsType type);
	void setDarkLineAbovePanelToolbar(HWND hwnd);
	void setDarkListView(HWND hwnd);

	void disableVisualStyle(HWND hwnd, bool doDisable);
	void calculateTreeViewStyle();
	void setTreeViewStyle(HWND hwnd);
	bool isThemeDark();
	void setBorder(HWND hwnd, bool border = true);

	LRESULT onCtlColor(HDC hdc);
	LRESULT onCtlColorSofter(HDC hdc);
	LRESULT onCtlColorDarker(HDC hdc);
	LRESULT onCtlColorError(HDC hdc);
	LRESULT onCtlColorDarkerBGStaticText(HDC hdc, bool isTextEnabled);
	INT_PTR onCtlColorListbox(WPARAM wParam, LPARAM lParam);
}
