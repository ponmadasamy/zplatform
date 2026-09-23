/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zephyr display driver for STM3210E-EVAL MB895 (FSMC bank4).
 */

#define DT_DRV_COMPAT zplatform_mb895_lcd

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <zplatform/mb895_lcd.h>

LOG_MODULE_REGISTER(display_mb895, CONFIG_DISPLAY_LOG_LEVEL);

struct mb895_config {
	uint16_t width;
	uint16_t height;
};

struct mb895_data {
	enum display_pixel_format pixfmt;
};

static int mb895_blanking_on(const struct device *dev)
{
	ARG_UNUSED(dev);
	return zplatform_mb895_lcd_blanking_on();
}

static int mb895_blanking_off(const struct device *dev)
{
	ARG_UNUSED(dev);
	return zplatform_mb895_lcd_blanking_off();
}

static int mb895_write(const struct device *dev, const uint16_t x,
		       const uint16_t y,
		       const struct display_buffer_descriptor *desc,
		       const void *buf)
{
	const struct mb895_config *cfg = dev->config;
	const uint16_t *pixels = buf;

	if (desc == NULL || buf == NULL) {
		return -EINVAL;
	}
	if (desc->width == 0U || desc->height == 0U) {
		return -EINVAL;
	}
	if (desc->pitch < desc->width) {
		return -EINVAL;
	}
	if ((uint32_t)desc->pitch * 2U * desc->height > desc->buf_size) {
		return -EINVAL;
	}
	if ((uint32_t)x + desc->width > cfg->width ||
	    (uint32_t)y + desc->height > cfg->height) {
		return -EINVAL;
	}

	LOG_DBG("write %ux%u @ %u,%u pitch=%u", desc->width, desc->height, x, y,
		desc->pitch);

	return zplatform_mb895_lcd_write_rect(x, y, desc->width, desc->height,
					      desc->pitch, pixels);
}

static void mb895_get_capabilities(const struct device *dev,
				   struct display_capabilities *caps)
{
	const struct mb895_config *cfg = dev->config;
	struct mb895_data *data = dev->data;

	caps->x_resolution = cfg->width;
	caps->y_resolution = cfg->height;
	caps->supported_pixel_formats = PIXEL_FORMAT_RGB_565;
	caps->screen_info = 0;
	caps->current_pixel_format = data->pixfmt;
	caps->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

static int mb895_set_pixel_format(const struct device *dev,
				  const enum display_pixel_format pf)
{
	struct mb895_data *data = dev->data;

	if (pf != PIXEL_FORMAT_RGB_565) {
		LOG_ERR("unsupported pixel format %d", pf);
		return -ENOTSUP;
	}
	data->pixfmt = pf;
	return 0;
}

static int mb895_init(const struct device *dev)
{
	struct mb895_data *data = dev->data;
	uint16_t id = 0;
	int rc;

	data->pixfmt = PIXEL_FORMAT_RGB_565;

	rc = zplatform_mb895_lcd_init(&id);
	if (rc < 0) {
		LOG_ERR("MB895 init failed (%d) ID 0x%04x", rc, id);
		return rc;
	}

	LOG_INF("MB895 ready ID 0x%04x (%s)", id,
		zplatform_mb895_lcd_id_name(id));

	/* Match ili9xxx: leave blanking on until the app calls blanking_off. */
	(void)zplatform_mb895_lcd_blanking_on();
	return 0;
}

static DEVICE_API(display, mb895_api) = {
	.blanking_on = mb895_blanking_on,
	.blanking_off = mb895_blanking_off,
	.write = mb895_write,
	.get_capabilities = mb895_get_capabilities,
	.set_pixel_format = mb895_set_pixel_format,
};

#define MB895_INIT(inst)                                                       \
	static const struct mb895_config mb895_cfg_##inst = {                  \
		.width = DT_INST_PROP(inst, width),                            \
		.height = DT_INST_PROP(inst, height),                          \
	};                                                                     \
	static struct mb895_data mb895_data_##inst;                            \
	DEVICE_DT_INST_DEFINE(inst, mb895_init, NULL, &mb895_data_##inst,      \
			      &mb895_cfg_##inst, POST_KERNEL,                  \
			      CONFIG_DISPLAY_INIT_PRIORITY, &mb895_api)

DT_INST_FOREACH_STATUS_OKAY(MB895_INIT)
