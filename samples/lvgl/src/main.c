/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Brand splash sample (logo + palette). Full EVK UI is demo/stm3210e_eval.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include <lvgl.h>

#include <zplatform/brand.h>
#include <zplatform/fsmc_sram.h>

int main(void)
{
	const struct device *disp = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	lv_obj_t *scr;
	lv_obj_t *brand;
	lv_obj_t *tag;
	int rc;

	printk("LVGL splash sample\n");

	if (!device_is_ready(disp)) {
		printk("[FAIL] display not ready\n");
		return 0;
	}

	if (!zplatform_fsmc_sram_ready() &&
	    zplatform_fsmc_sram_init() != 0) {
		printk("[FAIL] FSMC SRAM init\n");
		return 0;
	}
	printk("[PASS] EXT_SRAM ready for LVGL buffers\n");

	scr = lv_screen_active();
	lv_obj_set_style_bg_color(scr, lv_color_hex(ZPLATFORM_COLOR_MIDNIGHT), 0);
	lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

	brand = lv_label_create(scr);
	lv_label_set_text(brand, ZPLATFORM_BRAND_NAME);
	lv_obj_set_style_text_font(brand, &lv_font_montserrat_28, 0);
	lv_obj_set_style_text_color(brand, lv_color_hex(ZPLATFORM_COLOR_SNOW), 0);
	lv_obj_align(brand, LV_ALIGN_CENTER, 0, -20);

	tag = lv_label_create(scr);
	lv_label_set_text(tag, ZPLATFORM_BRAND_TAGLINE);
	lv_obj_set_style_text_font(tag, &lv_font_montserrat_14, 0);
	lv_obj_set_style_text_color(tag, lv_color_hex(ZPLATFORM_COLOR_CYAN), 0);
	lv_obj_align(tag, LV_ALIGN_CENTER, 0, 16);

	lv_timer_handler();
	rc = display_blanking_off(disp);
	if (rc < 0 && rc != -ENOSYS) {
		printk("[FAIL] blanking_off (%d)\n", rc);
		return 0;
	}

	printk("[PASS] LVGL splash on MB895\n");

	while (1) {
		uint32_t sleep_ms = lv_timer_handler();

		k_msleep(sleep_ms == 0U ? 5U : MIN(sleep_ms, 50U));
	}
}
