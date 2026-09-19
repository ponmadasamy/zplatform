/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Board-agnostic sensor sample. Uses the first available lm75-compatible
 * node when present (STLM75 on STM3210E-EVAL).
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>

#if DT_HAS_COMPAT_STATUS_OKAY(lm75)
#define SENSOR_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(lm75)
#else
#define SENSOR_NODE DT_INVALID_NODE
#endif

int main(void)
{
	printk("Sensor sample\n");

#if !DT_NODE_EXISTS(SENSOR_NODE)
	printk("[SKIP] No lm75-compatible sensor in DeviceTree\n");
	return 0;
#else
	const struct device *dev = DEVICE_DT_GET(SENSOR_NODE);
	struct sensor_value val;

	if (!device_is_ready(dev)) {
		printk("[FAIL] Sensor device not ready\n");
		return 0;
	}

	printk("[PASS] Sensor device ready (%s)\n", dev->name);

	while (1) {
		if (sensor_sample_fetch(dev) == 0 &&
		    sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &val) == 0) {
			printk("Temperature: %d.%06d C\n", val.val1, val.val2);
		} else {
			printk("[FAIL] Sensor read\n");
		}
		k_msleep(1000);
	}
#endif

	return 0;
}
