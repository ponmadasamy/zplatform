/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Board-agnostic PWM sample. Uses alias pwm-led0 when present
 * (TIM1_CH1 / PA8 on STM3210E-EVAL).
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/sys/printk.h>

#if DT_NODE_HAS_STATUS(DT_ALIAS(pwm_led0), okay)
#define HAS_PWM_LED0 1
#else
#define HAS_PWM_LED0 0
#endif

int main(void)
{
	printk("PWM sample\n");

#if !HAS_PWM_LED0
	printk("[SKIP] No pwm-led0 alias in DeviceTree\n");
	return 0;
#else
	static const struct pwm_dt_spec pwm = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led0));
	const uint32_t period = PWM_MSEC(20);
	uint32_t pulse = period / 2U;
	uint8_t step = 0;
	int err;

	if (!pwm_is_ready_dt(&pwm)) {
		printk("[FAIL] PWM device not ready\n");
		return 0;
	}

	err = pwm_set_dt(&pwm, period, pulse);
	if (err) {
		printk("[FAIL] PWM set (%d)\n", err);
		return 0;
	}

	printk("[PASS] PWM ch%d %u/%u ns (50%%)\n",
	       pwm.channel, pulse, period);

	/* Duty sweep so a scope on PA8 shows an obvious changing waveform. */
	while (1) {
		step = (step + 1U) % 5U;
		pulse = (period * (step + 1U)) / 6U;
		err = pwm_set_dt(&pwm, period, pulse);
		if (err) {
			printk("[FAIL] PWM set (%d)\n", err);
		} else {
			printk("PWM duty %u/%u ns\n", pulse, period);
		}
		k_msleep(1000);
	}
#endif

	return 0;
}
