/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * UI shell: joystick navigation across concept screens.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include <lvgl.h>

#include <zplatform/fsmc_sram.h>

#include "../demo_internal.h"
#include "ui_screens.h"
#include "ui_theme.h"

#if !defined(CONFIG_BOARD_STM3210E_EVAL)
#error "demo UI is for stm3210e_eval"
#endif

static const struct gpio_dt_spec joy_left =
	GPIO_DT_SPEC_GET_OR(DT_ALIAS(joy_left), gpios, {0});
static const struct gpio_dt_spec joy_right =
	GPIO_DT_SPEC_GET_OR(DT_ALIAS(joy_right), gpios, {0});
static const struct gpio_dt_spec joy_sel =
	GPIO_DT_SPEC_GET_OR(DT_ALIAS(joy_select), gpios, {0});

static lv_obj_t *screens[UI_SCREEN_COUNT];
static enum ui_screen_id cur;
static bool prev_left;
static bool prev_right;
static bool prev_sel;

static bool pin_active(const struct gpio_dt_spec *g)
{
	if (!g->port || !gpio_is_ready_dt(g)) {
		return false;
	}
	return gpio_pin_get_dt(g) > 0;
}

static void show_screen(enum ui_screen_id id)
{
	cur = id;
	lv_screen_load(screens[id]);
}

static void nav_poll(uint32_t now_ms)
{
	static uint32_t last_nav_ms;
	bool left = pin_active(&joy_left);
	bool right = pin_active(&joy_right);
	bool sel = pin_active(&joy_sel);
	bool edge = (right && !prev_right) || (left && !prev_left) ||
		    (sel && !prev_sel);

	prev_left = left;
	prev_right = right;
	prev_sel = sel;

	if (!edge) {
		return;
	}
	/* Debounce navigation edges (~180 ms). */
	if (last_nav_ms != 0U && (now_ms - last_nav_ms) < 180U) {
		return;
	}
	last_nav_ms = now_ms;

	if (right) {
		show_screen((enum ui_screen_id)((cur + 1) % UI_SCREEN_COUNT));
	} else if (left) {
		show_screen((enum ui_screen_id)((cur + UI_SCREEN_COUNT - 1) %
						UI_SCREEN_COUNT));
	} else if (sel && cur == UI_SCREEN_SPLASH) {
		show_screen(UI_SCREEN_IO);
	}
}

void demo_ui_run(void)
{
	const struct device *disp = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	int rc;

	printk("UI\n");

	if (!device_is_ready(disp)) {
		printk(" [FAIL] display not ready — UI aborted\n");
		return;
	}

	if (!zplatform_fsmc_sram_ready()) {
		rc = zplatform_fsmc_sram_init();
		if (rc != 0) {
			printk(" [FAIL] EXT_SRAM (%d)\n", rc);
			return;
		}
	}

	if (joy_left.port) {
		(void)gpio_pin_configure_dt(&joy_left, GPIO_INPUT);
	}
	if (joy_right.port) {
		(void)gpio_pin_configure_dt(&joy_right, GPIO_INPUT);
	}
	if (joy_sel.port) {
		(void)gpio_pin_configure_dt(&joy_sel, GPIO_INPUT);
	}

	for (int i = 0; i < UI_SCREEN_COUNT; i++) {
		/* NULL parent → top-level screen (not a child of active). */
		screens[i] = lv_obj_create(NULL);
		lv_obj_set_size(screens[i], LV_PCT(100), LV_PCT(100));
		lv_obj_set_style_border_width(screens[i], 0, 0);
		lv_obj_set_style_pad_all(screens[i], 0, 0);
		lv_obj_set_style_radius(screens[i], 0, 0);
		ui_style_screen(screens[i]);
	}

	ui_splash_create(screens[UI_SCREEN_SPLASH]);
	ui_io_create(screens[UI_SCREEN_IO]);
	ui_peri_create(screens[UI_SCREEN_PERI]);
	ui_threads_create(screens[UI_SCREEN_THREADS]);

	show_screen(UI_SCREEN_SPLASH);

	lv_timer_handler();
	rc = display_blanking_off(disp);
	if (rc < 0 && rc != -ENOSYS) {
		printk(" [FAIL] blanking_off (%d)\n", rc);
		return;
	}

	printk(" [PASS] LVGL UI (Joy L/R navigate)\n");

	while (1) {
		uint32_t now = k_uptime_get_32();
		uint32_t sleep_ms;

		nav_poll(now);

		switch (cur) {
		case UI_SCREEN_SPLASH:
			ui_splash_tick(now);
			break;
		case UI_SCREEN_IO:
			ui_io_tick(now);
			break;
		case UI_SCREEN_PERI:
			ui_peri_tick(now);
			break;
		case UI_SCREEN_THREADS:
			ui_threads_tick(now);
			break;
		default:
			break;
		}

		sleep_ms = lv_timer_handler();
		k_msleep(sleep_ms == 0U ? 10U : MIN(sleep_ms, 40U));
	}
}
