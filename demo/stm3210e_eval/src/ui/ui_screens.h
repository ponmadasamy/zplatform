/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UI_SCREENS_H_
#define UI_SCREENS_H_

#include <lvgl.h>

enum ui_screen_id {
	UI_SCREEN_SPLASH = 0,
	UI_SCREEN_IO,
	UI_SCREEN_PERI,
	UI_SCREEN_THREADS,
	UI_SCREEN_COUNT,
};

void ui_splash_create(lv_obj_t *parent);
void ui_splash_tick(uint32_t now_ms);

void ui_io_create(lv_obj_t *parent);
void ui_io_tick(uint32_t now_ms);

void ui_peri_create(lv_obj_t *parent);
void ui_peri_tick(uint32_t now_ms);

void ui_threads_create(lv_obj_t *parent);
void ui_threads_tick(uint32_t now_ms);

#endif /* UI_SCREENS_H_ */
