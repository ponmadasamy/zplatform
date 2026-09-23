/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * STM3210E-EVAL RTC as Zephyr counter (UM0488 §2.3).
 * STM32F1 has no calendar RTC driver in Zephyr — use counter ticks.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/sys/printk.h>

int main(void)
{
	const struct device *rtc = DEVICE_DT_GET(DT_NODELABEL(rtc));
	uint32_t ticks = 0;
	int rc;

	printk("RTC sample\n");

	if (!device_is_ready(rtc)) {
		printk("[FAIL] RTC counter not ready\n");
		return 0;
	}

	rc = counter_start(rtc);
	if (rc < 0 && rc != -EALREADY) {
		printk("[FAIL] RTC start (%d)\n", rc);
		return 0;
	}

	printk("[PASS] RTC counter (F1 / LSI) freq=%u Hz\n",
	       counter_get_frequency(rtc));

	while (1) {
		if (counter_get_value(rtc, &ticks) == 0) {
			printk("RTC ticks %u\n", ticks);
		} else {
			printk("[FAIL] RTC get_value\n");
		}
		k_msleep(1000);
	}

	return 0;
}
