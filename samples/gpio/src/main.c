/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led0), gpios, {0});
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw0), gpios, {0});

int main(void)
{
	printk("GPIO sample\n");

	if (!led.port || !gpio_is_ready_dt(&led)) {
		printk("[SKIP] led0 not available\n");
		return 0;
	}

	gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);

	if (button.port && gpio_is_ready_dt(&button)) {
		gpio_pin_configure_dt(&button, GPIO_INPUT);
		printk("[PASS] led0 + sw0 ready\n");
	} else {
		printk("[PASS] led0 ready (no sw0)\n");
	}

	while (1) {
		int on = 1;

		if (button.port && gpio_is_ready_dt(&button)) {
			on = gpio_pin_get_dt(&button) ? 1 : 0;
			gpio_pin_set_dt(&led, on);
		} else {
			gpio_pin_toggle_dt(&led);
		}
		k_msleep(200);
	}

	return 0;
}
