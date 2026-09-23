/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Board-agnostic LCD sample via Zephyr display API
 * (zephyr,display → MB895 on STM3210E-EVAL).
 */

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include <zplatform/mb895_lcd.h>

#if DT_HAS_CHOSEN(zephyr_display)
#define DISPLAY_NODE DT_CHOSEN(zephyr_display)
#else
#define DISPLAY_NODE DT_INVALID_NODE
#endif

int main(void)
{
	printk("LCD sample\n");

#if !DT_NODE_EXISTS(DISPLAY_NODE)
	printk("[SKIP] No zephyr,display in DeviceTree\n");
	return 0;
#else
	const struct device *disp = DEVICE_DT_GET(DISPLAY_NODE);
	struct display_capabilities caps;
	struct display_buffer_descriptor desc;
	uint16_t line[ZPLATFORM_MB895_LCD_WIDTH];
	int rc;

	if (!device_is_ready(disp)) {
		printk("[FAIL] display device not ready\n");
		return 0;
	}

	display_get_capabilities(disp, &caps);
	printk("[PASS] display %ux%u fmt=0x%x\n", caps.x_resolution,
	       caps.y_resolution, caps.current_pixel_format);

#if defined(CONFIG_ZPLATFORM_MB895_LCD)
	printk("[PASS] LCD ID 0x%04x (%s)\n", zplatform_mb895_lcd_id(),
	       zplatform_mb895_lcd_id_name(zplatform_mb895_lcd_id()));
#endif

	desc.buf_size = sizeof(line);
	desc.width = caps.x_resolution;
	desc.height = 1;
	desc.pitch = caps.x_resolution;
	desc.frame_incomplete = false;

	/* Red band top half of screen via display_write. */
	for (uint16_t i = 0; i < caps.x_resolution; i++) {
		line[i] = ZPLATFORM_RGB565(255, 0, 0);
	}
	for (uint16_t y = 0; y < caps.y_resolution / 2U; y++) {
		rc = display_write(disp, 0, y, &desc, line);
		if (rc < 0) {
			printk("[FAIL] display_write red (%d)\n", rc);
			return 0;
		}
	}

	/* Blue band bottom half. */
	for (uint16_t i = 0; i < caps.x_resolution; i++) {
		line[i] = ZPLATFORM_RGB565(0, 90, 180);
	}
	for (uint16_t y = caps.y_resolution / 2U; y < caps.y_resolution; y++) {
		rc = display_write(disp, 0, y, &desc, line);
		if (rc < 0) {
			printk("[FAIL] display_write blue (%d)\n", rc);
			return 0;
		}
	}

	rc = display_blanking_off(disp);
	if (rc < 0) {
		printk("[FAIL] blanking_off (%d)\n", rc);
		return 0;
	}

	printk("[PASS] display_write RGB565 + blanking_off\n");
	return 0;
#endif
}
