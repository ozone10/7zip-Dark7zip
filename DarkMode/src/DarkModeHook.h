// SPDX-License-Identifier: MPL-2.0

/*
 * Copyright (c) 2025 ozone10
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */


#pragma once

#include <windows.h>

namespace dmlib_hook
{
#if defined(_DARKMODELIB_USE_SCROLLBAR_FIX) && (_DARKMODELIB_USE_SCROLLBAR_FIX > 0)
	bool LoadOpenNcThemeData(const HMODULE& hUxtheme);
	void EnableDarkScrollBarForWindowAndChildren(HWND hWnd);
	void FixDarkScrollBar();
#endif

	void SetMySysColor(int nIndex, COLORREF clr);
	bool HookSysColor();
	void UnhookSysColor();

	bool HookThemeColor();
	void UnhookThemeColor();
} // namespace dmlib_hook
