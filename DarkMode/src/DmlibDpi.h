// SPDX-License-Identifier: MPL-2.0

/*
 * Copyright (c) 2025 ozone10
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

// Based on parts of the Notepad++ dpi code licensed under GPLv3.
// Originally by ozone10.


#pragma once

#include <windows.h>

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif

#if 0 // maybe for future hidpi enhancement
#ifndef WM_DPICHANGED_BEFOREPARENT
#define WM_DPICHANGED_BEFOREPARENT 0x02E2
#endif
#endif

#ifndef WM_DPICHANGED_AFTERPARENT
#define WM_DPICHANGED_AFTERPARENT 0x02E3
#endif

#if 0 // maybe for future hidpi enhancement
#ifndef WM_GETDPISCALEDSIZE
#define WM_GETDPISCALEDSIZE 0x02E4
#endif
#endif

namespace dmlib_dpi
{
	bool InitDpiAPI();

	[[nodiscard]] UINT GetDpiForSystem();
	[[nodiscard]] UINT GetDpiForWindow(HWND hWnd);
	[[nodiscard]] inline UINT GetDpiForParent(HWND hWnd)
	{
		return dmlib_dpi::GetDpiForWindow(::GetParent(hWnd));
	}

	[[nodiscard]] int GetSystemMetricsForDpi(int nIndex, UINT dpi);

	[[nodiscard]] inline int scale(int x, UINT toDpi, UINT fromDpi)
	{
		return ::MulDiv(x, static_cast<int>(toDpi), static_cast<int>(fromDpi));
	}

	[[nodiscard]] inline int scale(int x, UINT dpi)
	{
		return dmlib_dpi::scale(x, dpi, USER_DEFAULT_SCREEN_DPI);
	}

	[[nodiscard]] inline int unscale(int x, UINT dpi)
	{
		return dmlib_dpi::scale(x, USER_DEFAULT_SCREEN_DPI, dpi);
	}

	[[nodiscard]] inline int scale(int x, HWND hWnd)
	{
		return dmlib_dpi::scale(x, dmlib_dpi::GetDpiForWindow(hWnd), USER_DEFAULT_SCREEN_DPI);
	}

	[[nodiscard]] inline int unscale(int x, HWND hWnd)
	{
		return dmlib_dpi::scale(x, USER_DEFAULT_SCREEN_DPI, dmlib_dpi::GetDpiForWindow(hWnd));
	}

	[[nodiscard]] inline int scaleFont(int pt, UINT dpi)
	{
		return -(dmlib_dpi::scale(pt, dpi, 72));
	}

	[[nodiscard]] inline int scaleFont(int pt, HWND hWnd)
	{
		return -(dmlib_dpi::scale(pt, dmlib_dpi::GetDpiForWindow(hWnd), 72));
	}
} // namespace dmlib_dpi
