// SPDX-License-Identifier: MPL-2.0

/*
 * Copyright (c) 2025 ozone10
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */


#include "StdAfx.h"

#include "DmlibPaintHelper.h"

#include <windows.h>

/**
 * @brief Paints a rounded rectangle using the specified pen and brush.
 *
 * Draws a rounded rectangle defined by `rect`, using the provided pen (`hpen`) and brush (`hBrush`)
 * for the edge and fill, respectively. Preserves previous GDI object selections.
 *
 * @param[in]   hdc     Handle to the device context.
 * @param[in]   rect    Rectangle bounds for the shape.
 * @param[in]   hpen    Pen used to draw the edge.
 * @param[in]   hBrush  Brush used to inner fill.
 * @param[in]   width   Horizontal corner radius.
 * @param[in]   height  Vertical corner radius.
 */
void dmlib_paint::paintRoundRect(
	HDC hdc,
	const RECT& rect,
	HPEN hpen,
	HBRUSH hBrush,
	int width,
	int height
)
{
	auto holdBrush = ::SelectObject(hdc, hBrush);
	auto holdPen = ::SelectObject(hdc, hpen);
	::RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, width, height);
	::SelectObject(hdc, holdBrush);
	::SelectObject(hdc, holdPen);
}

/**
 * @brief Paints a rectangle using the specified pen and brush.
 *
 * Draws a rectangle defined by `rect`, using the provided pen (`hpen`) and brush (`hBrush`)
 * for the edge and fill, respectively. Preserves previous GDI object selections.
 * Forwards to `dmlib_paint::paintRoundRect` with `width` and `height` parameters with `0` value.
 *
 * @param[in]   hdc     Handle to the device context.
 * @param[in]   rect    Rectangle bounds for the shape.
 * @param[in]   hpen    Pen used to draw the edge.
 * @param[in]   hBrush  Brush used to inner fill.
 *
 * @see dmlib_paint::paintRoundRect()
 */
void dmlib_paint::paintRect(
	HDC hdc,
	const RECT& rect,
	HPEN hpen,
	HBRUSH hBrush
)
{
	dmlib_paint::paintRoundRect(hdc, rect, hpen, hBrush, 0, 0);
}

/**
 * @brief Paints an unfilled rounded rectangle (frame only).
 *
 * Forwards to `dmlib_paint::paintRoundRect` and uses a `NULL_BRUSH`
 * to omit the inner fill, drawing only the rounded frame.
 *
 * @param[in]   hdc     Handle to the device context.
 * @param[in]   rect    Rectangle bounds for the frame.
 * @param[in]   hpen    Pen used to draw the edge.
 * @param[in]   width   Horizontal corner radius.
 * @param[in]   height  Vertical corner radius.
 *
 * @see dmlib_paint::paintRoundRect()
 */
void dmlib_paint::paintRoundFrameRect(
	HDC hdc,
	const RECT& rect,
	HPEN hpen,
	int width,
	int height
)
{
	dmlib_paint::paintRoundRect(hdc, rect, hpen, static_cast<HBRUSH>(::GetStockObject(NULL_BRUSH)), width, height);
}

/**
 * @brief Paints an unfilled rectangle (frame only).
 *
 * Forwards to `dmlib_paint::paintRoundFrameRect`
 * with `width` and `height` parameters with `0` value.
 *
 * @param[in]   hdc     Handle to the device context.
 * @param[in]   rect    Rectangle bounds for the frame.
 * @param[in]   hpen    Pen used to draw the edge.
 *
 * @see dmlib_paint::paintRoundFrameRect()
 */
void dmlib_paint::paintFrameRect(HDC hdc, const RECT& rect, HPEN hpen)
{
	dmlib_paint::paintRoundFrameRect(hdc, rect, hpen, 0, 0);
}
