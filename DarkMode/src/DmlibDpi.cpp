// SPDX-License-Identifier: MPL-2.0

/*
 * Copyright (c) 2025 ozone10
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

// Based on parts of the Notepad++ dpi code licensed under GPLv3.
// Originally by ozone10.


#include "StdAfx.h"

#include "DmlibDpi.h"

#include <windows.h>

#include "ModuleHelper.h"

using fnGetDpiForSystem = auto (WINAPI*)(VOID) -> UINT;
using fnGetDpiForWindow = auto (WINAPI*)(HWND hwnd) -> UINT;
using fnGetSystemMetricsForDpi = auto (WINAPI*)(int nIndex, UINT dpi) -> int;

static fnGetDpiForSystem pfGetDpiForSystem = nullptr;
static fnGetDpiForWindow pfGetDpiForWindow = nullptr;
static fnGetSystemMetricsForDpi pfGetSystemMetricsForDpi = nullptr;

static UINT WINAPI DummyGetDpiForSystem()
{
	UINT dpi = USER_DEFAULT_SCREEN_DPI;
	HDC hdc = ::GetDC(nullptr);
	if (hdc != nullptr)
	{
		dpi = static_cast<UINT>(::GetDeviceCaps(hdc, LOGPIXELSX));
		::ReleaseDC(nullptr, hdc);
	}
	return dpi;
}

static UINT WINAPI DummyGetDpiForWindow([[maybe_unused]] HWND hwnd)
{
	return DummyGetDpiForSystem();
}

static int WINAPI DummyGetSystemMetricsForDpi(int nIndex, UINT dpi)
{
	return dmlib_dpi::scale(::GetSystemMetrics(nIndex), dpi);
}

bool dmlib_dpi::InitDpiAPI()
{
	HMODULE hUser32 = ::GetModuleHandleW(L"user32.dll");
	if (hUser32 != nullptr)
	{
		return
			dmlib_module::LoadFn(hUser32, pfGetDpiForSystem, "GetDpiForSystem", DummyGetDpiForSystem)
			&& dmlib_module::LoadFn(hUser32, pfGetDpiForWindow, "GetDpiForWindow", DummyGetDpiForWindow)
			&& dmlib_module::LoadFn(hUser32, pfGetSystemMetricsForDpi, "GetSystemMetricsForDpi", DummyGetSystemMetricsForDpi);
	}
	return false;
}

UINT dmlib_dpi::GetDpiForSystem()
{
	return pfGetDpiForSystem();
}

UINT dmlib_dpi::GetDpiForWindow(HWND hWnd)
{
	const auto dpi = pfGetDpiForWindow(hWnd);
	if (dpi > 0)
	{
		return dpi;
	}
	return dmlib_dpi::GetDpiForSystem();
}

int dmlib_dpi::GetSystemMetricsForDpi(int nIndex, UINT dpi)
{
	return pfGetSystemMetricsForDpi(nIndex, dpi);
}
