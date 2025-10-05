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
	/// Enables or disables dark mode support for a specific window.
	bool AllowDarkModeForWindow(HWND hWnd, bool allow);

	/// Determines if high contrast mode is currently active.
	[[nodiscard]] bool IsHighContrast();

#if defined(_DARKMODELIB_ALLOW_OLD_OS) && (_DARKMODELIB_ALLOW_OLD_OS > 0)
	/// Refreshes the title bar theme color for legacy systems.
	void RefreshTitleBarThemeColor(HWND hWnd);
#endif

	/// Checks whether a `WM_SETTINGCHANGE` message indicates a color scheme switch.
	[[nodiscard]] bool IsColorSchemeChangeMessage(LPARAM lParam);
	/// Checks whether a message indicates a color scheme switch.
	[[nodiscard]] bool IsColorSchemeChangeMessage(UINT uMsg, LPARAM lParam);

	/// Initializes undocumented dark mode API.
	void InitDarkMode();
	/// Enables or disables dark mode using undocumented API.
	void SetDarkMode(bool useDark, bool fixDarkScrollbar);

	/// Checks if the host OS is at least Windows 10.
	[[nodiscard]] bool IsWindows10();
	/// Checks if the host OS is at least Windows 11.
	[[nodiscard]] bool IsWindows11();
	/// Retrieves the current Windows build number.
	[[nodiscard]] DWORD GetWindowsBuildNumber();
} // namespace dmlib_win32api
