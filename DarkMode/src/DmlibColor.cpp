// SPDX-License-Identifier: MPL-2.0

/*
 * Copyright (c) 2025 ozone10
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */


#include "StdAfx.h"

#include "DmlibColor.h"

#include <windows.h>

#include "DarkModeSubclass.h"

DarkMode::Colors dmlib_color::getLightColors()
{
	return DarkMode::Colors{
		::GetSysColor(COLOR_3DFACE),        // background
		::GetSysColor(COLOR_WINDOW),        // ctrlBackground
		dmlib_color::HEXRGB(0xC0DCF3),      // hotBackground
		::GetSysColor(COLOR_3DFACE),        // dlgBackground
		dmlib_color::HEXRGB(0xA01000),      // errorBackground
		::GetSysColor(COLOR_WINDOWTEXT),    // textColor
		::GetSysColor(COLOR_BTNTEXT),       // darkerTextColor
		::GetSysColor(COLOR_GRAYTEXT),      // disabledTextColor
		::GetSysColor(COLOR_HOTLIGHT),      // linkTextColor
		dmlib_color::HEXRGB(0x8D8D8D),      // edgeColor
		::GetSysColor(COLOR_HIGHLIGHT),     // hotEdgeColor
		::GetSysColor(COLOR_GRAYTEXT)       // disabledEdgeColor
	};
}
