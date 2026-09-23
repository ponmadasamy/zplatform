/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Shared bring-up sample. Gates on DeviceTree aliases (led0, sw0), not board
 * Kconfig symbols. Optional per-board Kconfig: boards/<board>.conf
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/devicetree.h>

#include <zplatform/info.h>

#define SLEEP_MS 500

#if DT_NODE_EXISTS(DT_ALIAS(led0))
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
#endif

#if DT_NODE_EXISTS(DT_ALIAS(sw0))
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
#endif

int main(void)
{
#if DT_NODE_EXISTS(DT_ALIAS(led0))
	bool led_ok = false;
#endif
#if DT_NODE_EXISTS(DT_ALIAS(sw0))
	bool button_ok = false;
#endif

	printk("ZPlatform board bring-up\n");
	zplatform_print_banner();

#if DT_NODE_EXISTS(DT_ALIAS(led0))
	if (gpio_is_ready_dt(&led) &&
	    gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE) == 0) {
		led_ok = true;
		printk("[PASS] LED (led0)\n");
	} else {
		printk("[FAIL] LED configure\n");
	}
#else
	printk("[SKIP] LED - alias led0 not in DeviceTree\n");
#endif

#if DT_NODE_EXISTS(DT_ALIAS(sw0))
	if (gpio_is_ready_dt(&button) &&
	    gpio_pin_configure_dt(&button, GPIO_INPUT) == 0) {
		button_ok = true;
		printk("[PASS] BUTTON (sw0)\n");
	} else {
		printk("[FAIL] BUTTON configure\n");
	}
#else
	printk("[SKIP] BUTTON - alias sw0 not in DeviceTree\n");
#endif

	printk("[PASS] Console\n");
	printk("Bring-up ready. Blinking LED; button forces LED on when pressed.\n");

	while (1) {
#if DT_NODE_EXISTS(DT_ALIAS(led0))
		if (led_ok) {
			bool pressed = false;

#if DT_NODE_EXISTS(DT_ALIAS(sw0))
			if (button_ok) {
				pressed = gpio_pin_get_dt(&button) != 0;
			}
#endif
			if (pressed) {
				gpio_pin_set_dt(&led, 1);
			} else {
				gpio_pin_toggle_dt(&led);
			}
		}
#endif
		k_msleep(SLEEP_MS);
	}

	return 0;
}
