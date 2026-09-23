/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * IO screen — RV1 bar, key chips, graphical joystick pad.
 */

#include <stdio.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>

#include <lvgl.h>

#include "ui_theme.h"
#include "ui_screens.h"

#if DT_NODE_EXISTS(DT_PATH(zephyr_user)) && \
	DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
static const struct adc_dt_spec adc_rv1 =
	ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);
#define UI_HAS_RV1 1
#else
#define UI_HAS_RV1 0
#endif

static const struct gpio_dt_spec key_user =
	GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw0), gpios, {0});
static const struct gpio_dt_spec key_tamper =
	GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw1), gpios, {0});
static const struct gpio_dt_spec key_wakeup =
	GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw_wakeup), gpios, {0});
static const struct gpio_dt_spec joy_up =
	GPIO_DT_SPEC_GET_OR(DT_ALIAS(joy_up), gpios, {0});
static const struct gpio_dt_spec joy_down =
	GPIO_DT_SPEC_GET_OR(DT_ALIAS(joy_down), gpios, {0});
static const struct gpio_dt_spec joy_left =
	GPIO_DT_SPEC_GET_OR(DT_ALIAS(joy_left), gpios, {0});
static const struct gpio_dt_spec joy_right =
	GPIO_DT_SPEC_GET_OR(DT_ALIAS(joy_right), gpios, {0});
static const struct gpio_dt_spec joy_sel =
	GPIO_DT_SPEC_GET_OR(DT_ALIAS(joy_select), gpios, {0});

static lv_obj_t *rv1_bar;
static lv_obj_t *rv1_lbl;
static lv_obj_t *chip_user;
static lv_obj_t *chip_tamper;
static lv_obj_t *chip_wake;
static lv_obj_t *pad_u;
static lv_obj_t *pad_d;
static lv_obj_t *pad_l;
static lv_obj_t *pad_r;
static lv_obj_t *pad_s;

static bool adc_ok;
static int32_t last_raw = -1;
static uint8_t last_keys;
static uint8_t last_joy;
static uint32_t last_ms;

static bool key_down(const struct gpio_dt_spec *g)
{
	if (!g->port || !gpio_is_ready_dt(g)) {
		return false;
	}
	return gpio_pin_get_dt(g) > 0;
}

static void ensure_inputs(void)
{
	static bool done;

	if (done) {
		return;
	}
	done = true;

	const struct gpio_dt_spec *all[] = {
		&key_user, &key_tamper, &key_wakeup,
		&joy_up, &joy_down, &joy_left, &joy_right, &joy_sel,
	};

	for (size_t i = 0; i < ARRAY_SIZE(all); i++) {
		if (all[i]->port && gpio_is_ready_dt(all[i])) {
			(void)gpio_pin_configure_dt(all[i], GPIO_INPUT);
		}
	}

#if UI_HAS_RV1
	adc_ok = adc_is_ready_dt(&adc_rv1) &&
		 (adc_channel_setup_dt(&adc_rv1) == 0);
#endif
}

static lv_obj_t *make_chip(lv_obj_t *parent, const char *txt, int x, int y)
{
	lv_obj_t *box = lv_obj_create(parent);
	lv_obj_t *lab;

	lv_obj_set_size(box, 70, 28);
	lv_obj_set_pos(box, x, y);
	lv_obj_set_style_radius(box, 6, 0);
	lv_obj_set_style_border_width(box, 0, 0);
	lv_obj_set_style_pad_all(box, 2, 0);
	lv_obj_set_style_bg_color(box, lv_color_hex(0x1A2740), 0);
	lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);

	lab = lv_label_create(box);
	lv_label_set_text(lab, txt);
	lv_obj_set_style_text_font(lab, &lv_font_montserrat_12, 0);
	lv_obj_set_style_text_color(lab, ui_col_snow(), 0);
	lv_obj_center(lab);
	return box;
}

static lv_obj_t *make_pad(lv_obj_t *parent, const char *txt, int x, int y)
{
	lv_obj_t *box = lv_obj_create(parent);
	lv_obj_t *lab;

	lv_obj_set_size(box, 36, 36);
	lv_obj_set_pos(box, x, y);
	lv_obj_set_style_radius(box, 8, 0);
	lv_obj_set_style_border_width(box, 0, 0);
	lv_obj_set_style_pad_all(box, 0, 0);
	lv_obj_set_style_bg_color(box, lv_color_hex(0x1A2740), 0);
	lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);

	lab = lv_label_create(box);
	lv_label_set_text(lab, txt);
	lv_obj_set_style_text_font(lab, &lv_font_montserrat_12, 0);
	lv_obj_set_style_text_color(lab, ui_col_snow(), 0);
	lv_obj_center(lab);
	return box;
}

static void set_lit(lv_obj_t *obj, bool on, lv_color_t on_col)
{
	lv_obj_set_style_bg_color(obj, on ? on_col : lv_color_hex(0x1A2740), 0);
}

void ui_io_create(lv_obj_t *parent)
{
	lv_obj_t *title;
	lv_obj_t *keys_h;
	lv_obj_t *joy_h;
	int pad_x = 200;
	int pad_y = 100;

	ensure_inputs();
	ui_style_screen(parent);

	title = lv_label_create(parent);
	lv_label_set_text(title, "IO");
	lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
	lv_obj_set_style_text_color(title, ui_col_cyan(), 0);
	lv_obj_align(title, LV_ALIGN_TOP_LEFT, 8, 6);

	rv1_lbl = lv_label_create(parent);
	lv_label_set_text(rv1_lbl, "RV1  ----");
	lv_obj_set_style_text_font(rv1_lbl, &lv_font_montserrat_12, 0);
	lv_obj_set_style_text_color(rv1_lbl, ui_col_snow(), 0);
	lv_obj_align(rv1_lbl, LV_ALIGN_TOP_LEFT, 8, 28);

	rv1_bar = lv_bar_create(parent);
	lv_obj_set_size(rv1_bar, 300, 14);
	lv_obj_align(rv1_bar, LV_ALIGN_TOP_MID, 0, 44);
	lv_bar_set_range(rv1_bar, 0, 4095);
	lv_obj_set_style_bg_color(rv1_bar, lv_color_hex(0x1A2740),
				  LV_PART_MAIN);
	lv_obj_set_style_bg_color(rv1_bar, ui_col_azure(), LV_PART_INDICATOR);

	keys_h = lv_label_create(parent);
	lv_label_set_text(keys_h, "Keys");
	lv_obj_set_style_text_font(keys_h, &lv_font_montserrat_12, 0);
	lv_obj_set_style_text_color(keys_h, ui_col_purple(), 0);
	lv_obj_align(keys_h, LV_ALIGN_TOP_LEFT, 8, 68);

	chip_user = make_chip(parent, "User", 8, 88);
	chip_tamper = make_chip(parent, "Tamp", 86, 88);
	chip_wake = make_chip(parent, "Wake", 164, 88);

	joy_h = lv_label_create(parent);
	lv_label_set_text(joy_h, "Joystick");
	lv_obj_set_style_text_font(joy_h, &lv_font_montserrat_12, 0);
	lv_obj_set_style_text_color(joy_h, ui_col_purple(), 0);
	lv_obj_align(joy_h, LV_ALIGN_TOP_LEFT, 8, 124);

	/* Cross pad on the right (320×240) */
	pad_u = make_pad(parent, "U", pad_x + 40, pad_y);
	pad_l = make_pad(parent, "L", pad_x, pad_y + 36);
	pad_s = make_pad(parent, "S", pad_x + 40, pad_y + 36);
	pad_r = make_pad(parent, "R", pad_x + 80, pad_y + 36);
	pad_d = make_pad(parent, "D", pad_x + 40, pad_y + 72);

	lv_obj_t *hint = lv_label_create(parent);
	lv_label_set_text(hint, "Joy L/R change screen");
	lv_obj_set_style_text_font(hint, &lv_font_montserrat_12, 0);
	lv_obj_set_style_text_color(hint, ui_col_azure(), 0);
	lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -6);

	last_raw = -1;
	last_keys = 0xFF;
	last_joy = 0xFF;
	last_ms = 0;
}

void ui_io_tick(uint32_t now_ms)
{
	char buf[32];
	int32_t raw = 0;
	uint8_t keys;
	uint8_t joy;

	if (last_ms != 0U && (now_ms - last_ms) < 80U) {
		return;
	}
	last_ms = now_ms;

#if UI_HAS_RV1
	if (adc_ok) {
		uint16_t sample = 0;
		struct adc_sequence seq = {
			.buffer = &sample,
			.buffer_size = sizeof(sample),
		};

		(void)adc_sequence_init_dt(&adc_rv1, &seq);
		if (adc_read_dt(&adc_rv1, &seq) == 0) {
			raw = sample;
		}
	}
#endif

	/* Ignore tiny ADC noise to avoid bar flicker. */
	if (last_raw < 0 || (raw > last_raw + 12) || (raw + 12 < last_raw)) {
		last_raw = raw;
		lv_bar_set_value(rv1_bar, raw, LV_ANIM_OFF);
		snprintf(buf, sizeof(buf), "RV1  %4d", (int)raw);
		lv_label_set_text(rv1_lbl, buf);
	}

	keys = (uint8_t)((key_down(&key_user) ? 1 : 0) |
			 (key_down(&key_tamper) ? 2 : 0) |
			 (key_down(&key_wakeup) ? 4 : 0));
	if (keys != last_keys) {
		last_keys = keys;
		set_lit(chip_user, keys & 1, ui_col_azure());
		set_lit(chip_tamper, keys & 2, ui_col_amber());
		set_lit(chip_wake, keys & 4, ui_col_cyan());
	}

	joy = (uint8_t)((key_down(&joy_up) ? 1 : 0) |
			(key_down(&joy_down) ? 2 : 0) |
			(key_down(&joy_left) ? 4 : 0) |
			(key_down(&joy_right) ? 8 : 0) |
			(key_down(&joy_sel) ? 16 : 0));
	if (joy != last_joy) {
		last_joy = joy;
		set_lit(pad_u, joy & 1, ui_col_cyan());
		set_lit(pad_d, joy & 2, ui_col_cyan());
		set_lit(pad_l, joy & 4, ui_col_cyan());
		set_lit(pad_r, joy & 8, ui_col_cyan());
		set_lit(pad_s, joy & 16, ui_col_amber());
	}
}
