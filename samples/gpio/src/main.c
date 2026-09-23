/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Shared GPIO sample. STM3210E-EVAL gets the full LED-trigger + input UI;
 * other boards keep a simple led0 / sw0 loop.
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/devicetree.h>

#if defined(CONFIG_BOARD_STM3210E_EVAL)
void gpio_sample_stm3210e_eval(void);
#endif

#if !defined(CONFIG_BOARD_STM3210E_EVAL)
#if DT_NODE_EXISTS(DT_ALIAS(led0))
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
#endif

#if DT_NODE_EXISTS(DT_ALIAS(sw0))
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
#endif
#endif

int main(void)
{
#if defined(CONFIG_BOARD_STM3210E_EVAL)
	gpio_sample_stm3210e_eval();
	return 0;
#else
	printk("GPIO sample\n");

#if !DT_NODE_EXISTS(DT_ALIAS(led0))
	printk("[SKIP] led0 not in DeviceTree\n");
	return 0;
#else
	if (!gpio_is_ready_dt(&led)) {
		printk("[FAIL] led0 not ready\n");
		return 0;
	}

	gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);

#if DT_NODE_EXISTS(DT_ALIAS(sw0))
	if (gpio_is_ready_dt(&button)) {
		gpio_pin_configure_dt(&button, GPIO_INPUT);
		printk("[PASS] led0 + sw0 ready\n");
	} else {
		printk("[PASS] led0 ready (sw0 not ready)\n");
	}
#else
	printk("[PASS] led0 ready (no sw0)\n");
#endif

	while (1) {
#if DT_NODE_EXISTS(DT_ALIAS(sw0))
		if (gpio_is_ready_dt(&button)) {
			gpio_pin_set_dt(&led, gpio_pin_get_dt(&button) ? 1 : 0);
		} else {
			gpio_pin_toggle_dt(&led);
		}
#else
		gpio_pin_toggle_dt(&led);
#endif
		k_msleep(200);
	}
#endif
	return 0;
#endif
}
