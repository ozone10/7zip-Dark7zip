// MIT license
// Copyright(c) 2024-2025 ozone10

// Parts of code based on the win32-darkmode project
// https://github.com/ysc3839/win32-darkmode
// which is licensed under the MIT License. Copyright (c) 2019 Richard Yu

#pragma once

#include <windows.h>

extern bool g_darkModeSupported;
extern bool g_darkModeEnabled;


bool ShouldAppsUseDarkMode();
bool AllowDarkModeForWindow(HWND hWnd, bool allow);
bool IsHighContrast();
#if defined(_DARKMODELIB_ALLOW_OLD_OS)
void RefreshTitleBarThemeColor(HWND hWnd);
void SetTitleBarThemeColor(HWND hWnd, BOOL dark);
#endif
bool IsColorSchemeChangeMessage(LPARAM lParam);
bool IsColorSchemeChangeMessage(UINT message, LPARAM lParam);
void AllowDarkModeForApp(bool allow);
void EnableDarkScrollBarForWindowAndChildren(HWND hwnd);
void InitDarkMode();
void SetDarkMode(bool useDarkMode, bool fixDarkScrollbar);
bool IsWindows10();
bool IsWindows11();
DWORD GetWindowsBuildNumber();

void SetMySysColor(int nIndex, COLORREF clr);
bool HookSysColor();
void UnhookSysColor();
