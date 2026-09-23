/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * On-device ztest for the MB895 Zephyr display driver
 * (zephyr,display → zplatform,mb895-lcd).
 */

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/util.h>

#include <zplatform/mb895_lcd.h>

#if !DT_HAS_CHOSEN(zephyr_display)
#error "This test requires zephyr,display in DeviceTree"
#endif

#define DISPLAY_NODE DT_CHOSEN(zephyr_display)

static const struct device *disp_dev(void)
{
	return DEVICE_DT_GET(DISPLAY_NODE);
}

ZTEST(zplatform_display, test_device_ready)
{
	zassert_true(device_is_ready(disp_dev()), "display not ready");
}

ZTEST(zplatform_display, test_capabilities_rgb565)
{
	struct display_capabilities caps;

	display_get_capabilities(disp_dev(), &caps);
	zassert_equal(caps.x_resolution, ZPLATFORM_MB895_LCD_WIDTH, "width");
	zassert_equal(caps.y_resolution, ZPLATFORM_MB895_LCD_HEIGHT, "height");
	zassert_true((caps.supported_pixel_formats & PIXEL_FORMAT_RGB_565) != 0,
		     "RGB565 not supported");
	zassert_equal(caps.current_pixel_format, PIXEL_FORMAT_RGB_565, "fmt");
}

ZTEST(zplatform_display, test_controller_id)
{
	uint16_t id = zplatform_mb895_lcd_id();

	zassert_true(id == ZPLATFORM_MB895_ID_HX8347D ||
			     id == ZPLATFORM_MB895_ID_SPFD5408 ||
			     id == ZPLATFORM_MB895_ID_ILI9320 ||
			     id == ZPLATFORM_MB895_ID_ILI9325,
		     "unexpected LCD ID 0x%04x", id);
}

ZTEST(zplatform_display, test_set_pixel_format)
{
	const struct device *dev = disp_dev();
	int rc;

	rc = display_set_pixel_format(dev, PIXEL_FORMAT_RGB_565);
	zassert_equal(rc, 0, "RGB565 should be accepted");

	rc = display_set_pixel_format(dev, PIXEL_FORMAT_RGB_888);
	zassert_equal(rc, -ENOTSUP, "RGB888 must be rejected");
}

ZTEST(zplatform_display, test_write_rejects_bad_desc)
{
	const struct device *dev = disp_dev();
	uint16_t pix = ZPLATFORM_RGB565(255, 0, 0);
	struct display_buffer_descriptor desc = {
		.buf_size = sizeof(pix),
		.width = 1,
		.height = 1,
		.pitch = 1,
	};
	int rc;

	rc = display_write(dev, 0, 0, NULL, &pix);
	zassert_equal(rc, -EINVAL, "NULL desc");

	rc = display_write(dev, 0, 0, &desc, NULL);
	zassert_equal(rc, -EINVAL, "NULL buf");

	desc.pitch = 0;
	rc = display_write(dev, 0, 0, &desc, &pix);
	zassert_equal(rc, -EINVAL, "pitch < width");

	desc.pitch = 1;
	desc.buf_size = 1; /* too small for RGB565 */
	rc = display_write(dev, 0, 0, &desc, &pix);
	zassert_equal(rc, -EINVAL, "buf_size too small");
}

ZTEST(zplatform_display, test_write_rejects_oob)
{
	const struct device *dev = disp_dev();
	uint16_t pix = ZPLATFORM_RGB565(0, 255, 0);
	struct display_buffer_descriptor desc = {
		.buf_size = sizeof(pix),
		.width = 1,
		.height = 1,
		.pitch = 1,
	};
	int rc;

	rc = display_write(dev, ZPLATFORM_MB895_LCD_WIDTH, 0, &desc, &pix);
	zassert_equal(rc, -EINVAL, "x OOB");

	rc = display_write(dev, 0, ZPLATFORM_MB895_LCD_HEIGHT, &desc, &pix);
	zassert_equal(rc, -EINVAL, "y OOB");
}

ZTEST(zplatform_display, test_blanking_and_partial_write)
{
	const struct device *dev = disp_dev();
	uint16_t line[32];
	struct display_buffer_descriptor desc = {
		.buf_size = sizeof(line),
		.width = 32,
		.height = 1,
		.pitch = 32,
	};
	int rc;

	for (size_t i = 0; i < ARRAY_SIZE(line); i++) {
		line[i] = ZPLATFORM_RGB565(0, 90, 180);
	}

	rc = display_blanking_on(dev);
	zassert_equal(rc, 0, "blanking_on");

	rc = display_write(dev, 8, 8, &desc, line);
	zassert_equal(rc, 0, "partial write");

	rc = display_blanking_off(dev);
	zassert_equal(rc, 0, "blanking_off");
}

ZTEST(zplatform_display, test_fullscreen_band)
{
	const struct device *dev = disp_dev();
	uint16_t line[ZPLATFORM_MB895_LCD_WIDTH];
	struct display_buffer_descriptor desc = {
		.buf_size = sizeof(line),
		.width = ZPLATFORM_MB895_LCD_WIDTH,
		.height = 1,
		.pitch = ZPLATFORM_MB895_LCD_WIDTH,
	};
	int rc;

	for (uint16_t i = 0; i < ZPLATFORM_MB895_LCD_WIDTH; i++) {
		line[i] = ZPLATFORM_RGB565(255, 0, 0);
	}

	for (uint16_t y = 0; y < 16; y++) {
		rc = display_write(dev, 0, y, &desc, line);
		zassert_equal(rc, 0, "band write y=%u", y);
	}

	rc = display_blanking_off(dev);
	zassert_equal(rc, 0, "blanking_off after band");
}

ZTEST_SUITE(zplatform_display, NULL, NULL, NULL, NULL, NULL);
