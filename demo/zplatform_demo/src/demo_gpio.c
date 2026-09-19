/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

#include "demo_internal.h"

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led0), gpios, {0});
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw0), gpios, {0});

void demo_run_gpio(void)
{
	printk("GPIO\n");

	if (led.port && gpio_is_ready_dt(&led) &&
	    gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE) == 0) {
		printk(" [PASS] LED\n");
		gpio_pin_set_dt(&led, 1);
	} else {
		printk(" [SKIP] LED - not available\n");
	}

	if (button.port && gpio_is_ready_dt(&button) &&
	    gpio_pin_configure_dt(&button, GPIO_INPUT) == 0) {
		int state = gpio_pin_get_dt(&button);

		printk(" [PASS] USER BUTTON (raw=%d)\n", state);
	} else {
		printk(" [SKIP] USER BUTTON - not available\n");
	}

	printk("\n");
}
