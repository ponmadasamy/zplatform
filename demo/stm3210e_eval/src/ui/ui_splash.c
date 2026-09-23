/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Splash screen — brand logo.png (mark + wordmark + tagline).
 */

#include <lvgl.h>

#include "ui_theme.h"
#include "ui_screens.h"
#include "logo_splash.h"

void ui_splash_create(lv_obj_t *parent)
{
	lv_obj_t *img;
	lv_obj_t *hint;

	ui_style_screen(parent);

	img = lv_image_create(parent);
	lv_image_set_src(img, &logo_splash);
	lv_obj_align(img, LV_ALIGN_CENTER, 0, -8);

	hint = lv_label_create(parent);
	lv_label_set_text(hint, "Joy L/R: screens  |  Select: OK");
	lv_obj_set_style_text_font(hint, &lv_font_montserrat_12, 0);
	lv_obj_set_style_text_color(hint, ui_col_azure(), 0);
	lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -8);
}

void ui_splash_tick(uint32_t now_ms)
{
	ARG_UNUSED(now_ms);
}
