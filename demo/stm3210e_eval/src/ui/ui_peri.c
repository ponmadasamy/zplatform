/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Peripherals status — live EVK feature card (UM0488 inventory).
 */

#include <stdio.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/util.h>

#include <lvgl.h>
#include <zplatform/fsmc_sram.h>
#include <zplatform/mb895_lcd.h>
#include <zplatform/sdio_sd.h>

#include "ui_theme.h"
#include "ui_screens.h"

static lv_obj_t *body;
static uint32_t last_ms;

static void append_line(char *buf, size_t n, const char *line)
{
	size_t len = strlen(buf);

	if (len + 1U >= n) {
		return;
	}
	snprintf(buf + len, n - len, "%s\n", line);
}

void ui_peri_create(lv_obj_t *parent)
{
	lv_obj_t *title;

	ui_style_screen(parent);

	title = lv_label_create(parent);
	lv_label_set_text(title, "Peripherals (live)");
	lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
	lv_obj_set_style_text_color(title, ui_col_cyan(), 0);
	lv_obj_align(title, LV_ALIGN_TOP_LEFT, 8, 4);

	body = lv_label_create(parent);
	lv_label_set_text(body, "…");
	lv_obj_set_style_text_font(body, &lv_font_montserrat_12, 0);
	lv_obj_set_style_text_color(body, ui_col_snow(), 0);
	lv_obj_set_width(body, 304);
	lv_obj_align(body, LV_ALIGN_TOP_LEFT, 8, 26);

	lv_obj_t *hint = lv_label_create(parent);
	lv_label_set_text(hint, "Joy L/R change screen");
	lv_obj_set_style_text_font(hint, &lv_font_montserrat_12, 0);
	lv_obj_set_style_text_color(hint, ui_col_azure(), 0);
	lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -4);

	last_ms = 0;
}

void ui_peri_tick(uint32_t now_ms)
{
	char buf[520];
	char line[80];

	if (last_ms != 0U && (now_ms - last_ms) < 400U) {
		return;
	}
	last_ms = now_ms;
	buf[0] = '\0';

#if DT_HAS_COMPAT_STATUS_OKAY(lm75)
	{
		const struct device *dev =
			DEVICE_DT_GET(DT_COMPAT_GET_ANY_STATUS_OKAY(lm75));
		struct sensor_value val;

		if (device_is_ready(dev) &&
		    sensor_sample_fetch(dev) == 0 &&
		    sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &val) ==
			    0) {
			snprintf(line, sizeof(line), "STLM75  %d.%02d C",
				 val.val1, val.val2 / 10000);
		} else {
			snprintf(line, sizeof(line), "STLM75  --");
		}
		append_line(buf, sizeof(buf), line);
	}
#endif

#if DT_NODE_HAS_STATUS(DT_ALIAS(die_temp0), okay)
	{
		const struct device *dev = DEVICE_DT_GET(DT_ALIAS(die_temp0));
		struct sensor_value val;

		if (device_is_ready(dev) &&
		    sensor_sample_fetch(dev) == 0 &&
		    sensor_channel_get(dev, SENSOR_CHAN_DIE_TEMP, &val) == 0) {
			snprintf(line, sizeof(line), "DieTemp %d.%02d C",
				 val.val1, val.val2 / 10000);
		} else {
			snprintf(line, sizeof(line), "DieTemp --");
		}
		append_line(buf, sizeof(buf), line);
	}
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(rtc), okay)
	{
		const struct device *rtc = DEVICE_DT_GET(DT_NODELABEL(rtc));
		static bool started;
		uint32_t ticks = 0;

		if (device_is_ready(rtc)) {
			if (!started) {
				(void)counter_start(rtc);
				started = true;
			}
			if (counter_get_value(rtc, &ticks) == 0) {
				snprintf(line, sizeof(line), "RTC     %u t",
					 ticks);
			} else {
				snprintf(line, sizeof(line), "RTC     --");
			}
		} else {
			snprintf(line, sizeof(line), "RTC     --");
		}
		append_line(buf, sizeof(buf), line);
	}
#endif

#if DT_NODE_EXISTS(DT_PATH(zephyr_user)) && \
	DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
	{
		static const struct adc_dt_spec rv1 =
			ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);
		uint16_t sample = 0;
		struct adc_sequence seq = {
			.buffer = &sample,
			.buffer_size = sizeof(sample),
		};

		if (adc_is_ready_dt(&rv1) &&
		    adc_channel_setup_dt(&rv1) == 0 &&
		    adc_sequence_init_dt(&rv1, &seq) == 0 &&
		    adc_read_dt(&rv1, &seq) == 0) {
			snprintf(line, sizeof(line), "RV1     %u", sample);
		} else {
			snprintf(line, sizeof(line), "RV1     --");
		}
		append_line(buf, sizeof(buf), line);
	}
#endif

#if DT_NODE_EXISTS(DT_PATH(zephyr_user)) && \
	DT_NODE_HAS_PROP(DT_PATH(zephyr_user), dac)
	{
		const struct device *dac =
			DEVICE_DT_GET(DT_PHANDLE(DT_PATH(zephyr_user), dac));
		const uint8_t ch =
			DT_PROP(DT_PATH(zephyr_user), dac_channel_id);
		static bool dac_ready;
		static uint16_t dac_v;

		if (!dac_ready && device_is_ready(dac)) {
			struct dac_channel_cfg cfg = {
				.channel_id = ch,
				.resolution = DT_PROP(DT_PATH(zephyr_user),
						      dac_resolution),
				.buffered = true,
			};

			if (dac_channel_setup(dac, &cfg) == 0) {
				dac_ready = true;
			}
		}
		if (dac_ready) {
			dac_v = (dac_v + 256U) & 0x0FFFU;
			(void)dac_write_value(dac, ch, dac_v);
			snprintf(line, sizeof(line), "DAC1.1  %u", dac_v);
		} else {
			snprintf(line, sizeof(line), "DAC1.1  --");
		}
		append_line(buf, sizeof(buf), line);
	}
#endif

#if DT_HAS_CHOSEN(zephyr_canbus)
	{
		const struct device *can =
			DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
		static bool can_up;
		enum can_state st = CAN_STATE_STOPPED;
		struct can_bus_err_cnt err = {0};

		if (device_is_ready(can)) {
			if (!can_up) {
				(void)can_set_mode(can, CAN_MODE_LOOPBACK);
				if (can_start(can) == 0) {
					can_up = true;
				}
			}
			(void)can_get_state(can, &st, &err);
			snprintf(line, sizeof(line), "CAN     st=%d %s",
				 (int)st, can_up ? "loop" : "init");
		} else {
			snprintf(line, sizeof(line), "CAN     --");
		}
		append_line(buf, sizeof(buf), line);
	}
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(jedec_spi_nor)
	{
		const struct device *flash =
			DEVICE_DT_GET(DT_COMPAT_GET_ANY_STATUS_OKAY(jedec_spi_nor));
		uint8_t id[3] = {0};

		if (device_is_ready(flash) &&
		    flash_read_jedec_id(flash, id) == 0) {
			snprintf(line, sizeof(line), "SPI NOR %02X%02X%02X",
				 id[0], id[1], id[2]);
		} else {
			snprintf(line, sizeof(line), "SPI NOR --");
		}
		append_line(buf, sizeof(buf), line);
	}
#endif

#if defined(CONFIG_ZPLATFORM_FSMC_SRAM)
	snprintf(line, sizeof(line), "SRAM    %s",
		 zplatform_fsmc_sram_ready() ? "ready" : "init");
	append_line(buf, sizeof(buf), line);
#endif

#if defined(CONFIG_ZPLATFORM_SDIO_SD)
	if (!zplatform_sdio_sd_detected()) {
		append_line(buf, sizeof(buf), "microSD absent");
	} else if (zplatform_sdio_sd_ready() ||
		   zplatform_sdio_sd_init() == 0) {
		struct zplatform_sd_info info;

		if (zplatform_sdio_sd_get_info(&info) == 0) {
			snprintf(line, sizeof(line), "microSD %u blk",
				 info.log_block_nbr);
		} else {
			snprintf(line, sizeof(line), "microSD ready");
		}
		append_line(buf, sizeof(buf), line);
	} else {
		append_line(buf, sizeof(buf), "microSD fail");
	}
#endif

#if defined(CONFIG_ZPLATFORM_MB895_LCD)
	snprintf(line, sizeof(line), "LCD     0x%04X",
		 zplatform_mb895_lcd_id());
	append_line(buf, sizeof(buf), line);
#endif

	append_line(buf, sizeof(buf), "USB     see samples/usb_cdc");
	append_line(buf, sizeof(buf), "NAND/NOR/Audio Todo");
	lv_label_set_text(body, buf);
}
