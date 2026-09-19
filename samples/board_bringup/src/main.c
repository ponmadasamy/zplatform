/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Board-agnostic bring-up sample. Uses DeviceTree aliases (led0, sw0)
 * so the same application works across STM32 EVKs and custom boards.
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

#include <zplatform/info.h>

#define SLEEP_MS 500

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led0), gpios, {0});
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw0), gpios, {0});

int main(void)
{
	bool led_ok = false;
	bool button_ok = false;

	printk("ZPlatform board bring-up\n");
	zplatform_print_banner();

	if (led.port && gpio_is_ready_dt(&led)) {
		if (gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE) == 0) {
			led_ok = true;
			printk("[PASS] LED (led0)\n");
		} else {
			printk("[FAIL] LED configure\n");
		}
	} else {
		printk("[SKIP] LED - alias led0 not available\n");
	}

	if (button.port && gpio_is_ready_dt(&button)) {
		if (gpio_pin_configure_dt(&button, GPIO_INPUT) == 0) {
			button_ok = true;
			printk("[PASS] BUTTON (sw0)\n");
		} else {
			printk("[FAIL] BUTTON configure\n");
		}
	} else {
		printk("[SKIP] BUTTON - alias sw0 not available\n");
	}

	printk("[PASS] Console\n");
	printk("Bring-up ready. Blinking LED; button forces LED on when pressed.\n");

	while (1) {
		if (led_ok) {
			bool pressed = false;

			if (button_ok) {
				pressed = gpio_pin_get_dt(&button) != 0;
			}

			if (pressed) {
				gpio_pin_set_dt(&led, 1);
			} else {
				gpio_pin_toggle_dt(&led);
			}
		}
		k_msleep(SLEEP_MS);
	}

	return 0;
}
