/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/printk.h>

#include "demo_internal.h"

void demo_run_peripherals(void)
{
	printk("Peripherals\n");

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c1), okay)
	{
		const struct device *i2c = DEVICE_DT_GET(DT_NODELABEL(i2c1));

		if (device_is_ready(i2c)) {
			printk(" [PASS] I2C\n");
		} else {
			printk(" [FAIL] I2C - not ready\n");
		}
	}
#else
	printk(" [SKIP] I2C - not available\n");
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(spi1), okay)
	{
		const struct device *spi = DEVICE_DT_GET(DT_NODELABEL(spi1));

		if (device_is_ready(spi)) {
			printk(" [PASS] SPI\n");
		} else {
			printk(" [FAIL] SPI - not ready\n");
		}
	}
#else
	printk(" [SKIP] SPI - not available\n");
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(adc1), okay)
	{
		const struct device *adc = DEVICE_DT_GET(DT_NODELABEL(adc1));

		if (device_is_ready(adc)) {
			printk(" [PASS] ADC\n");
		} else {
			printk(" [FAIL] ADC - not ready\n");
		}
	}
#else
	printk(" [SKIP] ADC - not available\n");
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(pwm1), okay)
	{
		const struct device *pwm = DEVICE_DT_GET(DT_NODELABEL(pwm1));

		if (device_is_ready(pwm)) {
			printk(" [PASS] PWM\n");
		} else {
			printk(" [FAIL] PWM - not ready\n");
		}
	}
#else
	printk(" [SKIP] PWM - not available\n");
#endif

#if FIXED_PARTITION_EXISTS(storage_partition)
	printk(" [PASS] Flash (storage partition present)\n");
#elif defined(CONFIG_FLASH)
	printk(" [PASS] Flash (CONFIG_FLASH enabled)\n");
#else
	printk(" [SKIP] Flash - not available\n");
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(lm75)
	{
		const struct device *dev =
			DEVICE_DT_GET(DT_COMPAT_GET_ANY_STATUS_OKAY(lm75));
		struct sensor_value val;

		if (device_is_ready(dev) &&
		    sensor_sample_fetch(dev) == 0 &&
		    sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &val) == 0) {
			printk(" [PASS] Temperature sensor (%d.%06d C)\n",
			       val.val1, val.val2);
		} else if (device_is_ready(dev)) {
			printk(" [PASS] Temperature sensor (device ready; read needs HW)\n");
		} else {
			printk(" [FAIL] Temperature sensor\n");
		}
	}
#else
	printk(" [SKIP] Temperature sensor - not in DT\n");
#endif

	printk(" [SKIP] Ethernet - not available on STM32F103\n");
	printk("\n");
}
