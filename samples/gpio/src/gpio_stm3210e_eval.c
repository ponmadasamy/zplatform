/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * STM3210E-EVAL GPIO + LED triggers sample.
 *
 * LD1 heartbeat | LD2 cpu | LD3 timer | LD4 panic  (rotatable)
 * User Key      — pause / resume all LED patterns
 * Tamper        — STANDBY (power off); Wakeup (PA0/WKUP) resets and boots again
 * Joy Up/Down   — increase / decrease timer gap between pulses
 * Joy Right/Left— rotate triggers across LD1–LD4
 *
 * Requires JP4 open so PA0 is Wakeup / WKUP (not USART2 CTS).
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/poweroff.h>

#include <stm32_ll_pwr.h>
#include <stm32_ll_bus.h>

#include <zplatform/led_triggers.h>

#define DEBOUNCE_MS 180
#define TIMER_STEP_MS 50

#define BTN(_alias) GPIO_DT_SPEC_GET_OR(DT_ALIAS(_alias), gpios, {0})

static const struct gpio_dt_spec user_key = BTN(sw0);
static const struct gpio_dt_spec tamper = BTN(sw1);
static const struct gpio_dt_spec wakeup = BTN(sw_wakeup);
static const struct gpio_dt_spec joy_up = BTN(joy_up);
static const struct gpio_dt_spec joy_down = BTN(joy_down);
static const struct gpio_dt_spec joy_left = BTN(joy_left);
static const struct gpio_dt_spec joy_right = BTN(joy_right);

static bool btn_ready(const struct gpio_dt_spec *b)
{
	return b->port != NULL && gpio_is_ready_dt(b);
}

static int btn_setup_input(const struct gpio_dt_spec *b, const char *name)
{
	if (!btn_ready(b)) {
		printk(" [SKIP] %s\n", name);
		return -ENODEV;
	}
	if (gpio_pin_configure_dt(b, GPIO_INPUT) == 0) {
		printk(" [PASS] %s\n", name);
		return 0;
	}
	printk(" [FAIL] %s configure\n", name);
	return -EIO;
}

static bool btn_active(const struct gpio_dt_spec *b)
{
	if (!btn_ready(b)) {
		return false;
	}
	return gpio_pin_get_dt(b) != 0;
}

static void print_map(const char *prefix)
{
	char map[32];

	if (zplatform_led_triggers_format_map(map, sizeof(map)) > 0) {
		printk("%s map=[%s] timer_gap=%u ms%s\n", prefix, map,
		       zplatform_led_triggers_timer_ms(),
		       zplatform_led_triggers_paused() ? " PAUSED" : "");
	}
}

static void enter_standby(void)
{
	printk("Sleep: STANDBY (Tamper). Press Wakeup (B2) to reboot…\n");
	k_msleep(100);

	while (btn_ready(&wakeup) && btn_active(&wakeup)) {
		k_msleep(10);
	}

	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
	LL_PWR_ClearFlag_WU();
	LL_PWR_ClearFlag_SB();
	LL_PWR_EnableWakeUpPin(LL_PWR_WAKEUP_PIN1);

	sys_poweroff();
}

void gpio_sample_stm3210e_eval(void)
{
	int64_t cool_user = 0;
	int64_t cool_tamp = 0;
	int64_t cool_joy = 0;

	printk("GPIO sample (STM3210E-EVAL)\n");
	printk(" Controls: User=pause  Tamper=STANDBY  Wakeup=reboot\n");
	printk("           Joy U/D=timer gap  Joy L/R=rotate triggers\n");
	printk(" LEDs: LD1=HB LD2=CPU LD3=TIM LD4=PAN  (JP4 open for WKUP)\n\n");

	(void)btn_setup_input(&user_key, "User Key (B4)");
	(void)btn_setup_input(&tamper, "Tamper (B3)");
	(void)btn_setup_input(&wakeup, "Wakeup (B2)");
	(void)btn_setup_input(&joy_up, "Joystick Up");
	(void)btn_setup_input(&joy_down, "Joystick Down");
	(void)btn_setup_input(&joy_left, "Joystick Left");
	(void)btn_setup_input(&joy_right, "Joystick Right");

	if (zplatform_led_triggers_start() != 0) {
		printk(" [FAIL] LED triggers\n");
		return;
	}
	printk(" [PASS] LED triggers started\n");
	print_map(" ");

	while (1) {
		int64_t now = k_uptime_get();

		if (btn_active(&user_key) && now >= cool_user) {
			bool next = !zplatform_led_triggers_paused();

			zplatform_led_triggers_pause(next);
			printk("%s\n", next ? "LEDs paused" : "LEDs resumed");
			print_map(" ");
			cool_user = now + DEBOUNCE_MS;
		}

		if (btn_active(&tamper) && now >= cool_tamp) {
			cool_tamp = now + DEBOUNCE_MS;
			enter_standby();
		}

		if (now >= cool_joy) {
			if (btn_active(&joy_up)) {
				uint32_t ms = zplatform_led_triggers_timer_adjust(TIMER_STEP_MS);

				printk("Timer gap + → %u ms\n", ms);
				print_map(" ");
				cool_joy = now + DEBOUNCE_MS;
			} else if (btn_active(&joy_down)) {
				uint32_t ms = zplatform_led_triggers_timer_adjust(-TIMER_STEP_MS);

				printk("Timer gap − → %u ms\n", ms);
				print_map(" ");
				cool_joy = now + DEBOUNCE_MS;
			} else if (btn_active(&joy_right)) {
				zplatform_led_triggers_rotate(+1);
				print_map("Rotate right");
				cool_joy = now + DEBOUNCE_MS;
			} else if (btn_active(&joy_left)) {
				zplatform_led_triggers_rotate(-1);
				print_map("Rotate left");
				cool_joy = now + DEBOUNCE_MS;
			}
		}

		k_msleep(20);
	}
}
