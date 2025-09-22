// SPDX-License-Identifier: MPL-2.0

/*
 * Copyright (c) 2025 ozone10
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work from the win32-darkmode project:
 *  https://github.com/ysc3839/win32-darkmode
 *  which is covered by the MIT License.
 *  See LICENSE-win32-darkmode for more information.
 */


#pragma once

#include <windows.h>

extern bool g_darkModeSupported;
extern bool g_darkModeEnabled;

namespace dmlib_win32api
{
	bool AllowDarkModeForWindow(HWND hWnd, bool allow);
	[[nodiscard]] bool IsHighContrast();
#if defined(_DARKMODELIB_ALLOW_OLD_OS) && (_DARKMODELIB_ALLOW_OLD_OS > 0)
	void RefreshTitleBarThemeColor(HWND hWnd);
	void SetTitleBarThemeColor(HWND hWnd, BOOL dark);
#endif
	[[nodiscard]] bool IsColorSchemeChangeMessage(LPARAM lParam);
	[[nodiscard]] bool IsColorSchemeChangeMessage(UINT uMsg, LPARAM lParam);
	void AllowDarkModeForApp(bool allow);
	void InitDarkMode();
	void SetDarkMode(bool useDark, bool fixDarkScrollbar);
	[[nodiscard]] bool IsWindows10();
	[[nodiscard]] bool IsWindows11();
	[[nodiscard]] DWORD GetWindowsBuildNumber();
} // namespace dmlib_win32api
