/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Brand palette helpers for LVGL screens.
 */

#ifndef UI_THEME_H_
#define UI_THEME_H_

#include <lvgl.h>
#include <zplatform/brand.h>

static inline lv_color_t ui_col_midnight(void)
{
	return lv_color_hex(ZPLATFORM_COLOR_MIDNIGHT);
}

static inline lv_color_t ui_col_azure(void)
{
	return lv_color_hex(ZPLATFORM_COLOR_AZURE);
}

static inline lv_color_t ui_col_cyan(void)
{
	return lv_color_hex(ZPLATFORM_COLOR_CYAN);
}

static inline lv_color_t ui_col_purple(void)
{
	return lv_color_hex(ZPLATFORM_COLOR_PURPLE);
}

static inline lv_color_t ui_col_amber(void)
{
	return lv_color_hex(ZPLATFORM_COLOR_AMBER);
}

static inline lv_color_t ui_col_snow(void)
{
	return lv_color_hex(ZPLATFORM_COLOR_SNOW);
}

static inline void ui_style_screen(lv_obj_t *scr)
{
	lv_obj_set_style_bg_color(scr, ui_col_midnight(), 0);
	lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
}

#endif /* UI_THEME_H_ */
