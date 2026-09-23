/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * DAC1 CH1 (PA4) — UM0488 audio path / CN10. CH2 (PA5) shares SPI1 SCK.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/sys/printk.h>

#define USER_NODE DT_PATH(zephyr_user)

#if DT_NODE_HAS_PROP(USER_NODE, dac) && \
	DT_NODE_HAS_PROP(USER_NODE, dac_channel_id) && \
	DT_NODE_HAS_PROP(USER_NODE, dac_resolution)
#define HAS_DAC 1
#else
#define HAS_DAC 0
#endif

int main(void)
{
	printk("DAC sample\n");

#if !HAS_DAC
	printk("[SKIP] No dac props under zephyr,user\n");
	return 0;
#else
	const struct device *dac =
		DEVICE_DT_GET(DT_PHANDLE(USER_NODE, dac));
	const uint8_t ch = DT_PROP(USER_NODE, dac_channel_id);
	const uint8_t res = DT_PROP(USER_NODE, dac_resolution);
	struct dac_channel_cfg cfg = {
		.channel_id = ch,
		.resolution = res,
		.buffered = true,
	};
	const uint32_t maxv = (1U << res) - 1U;
	int rc;

	if (!device_is_ready(dac)) {
		printk("[FAIL] DAC not ready\n");
		return 0;
	}

	rc = dac_channel_setup(dac, &cfg);
	if (rc != 0) {
		printk("[FAIL] DAC setup (%d)\n", rc);
		return 0;
	}

	printk("[PASS] DAC ch%u %u-bit (PA4)\n", ch, res);

	while (1) {
		for (uint32_t v = 0; v <= maxv; v += (maxv / 16U) ? (maxv / 16U) : 1U) {
			rc = dac_write_value(dac, ch, v);
			if (rc != 0) {
				printk("[FAIL] DAC write (%d)\n", rc);
				return 0;
			}
			printk("DAC out %u/%u\n", v, maxv);
			k_msleep(200);
		}
	}
#endif

	return 0;
}
