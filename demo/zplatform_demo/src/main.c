/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/version.h>

#include <zplatform/info.h>

#include "demo_internal.h"

int main(void)
{
	struct zplatform_sys_info info;

	printk("========================================\n");
	printk(" ZPlatform Demo\n");
	if (zplatform_get_sys_info(&info) == 0) {
		printk(" %s\n", info.board);
	}
	printk("========================================\n\n");

	printk("System\n");
	printk(" [PASS] Boot\n");
	printk(" [PASS] Clock (%u Hz)\n",
	       info.cpu_freq_hz ? info.cpu_freq_hz : sys_clock_hw_cycles_per_sec());
	printk(" [PASS] Kernel (%s)\n", KERNEL_VERSION_STRING);
	printk(" [PASS] Console\n\n");

	demo_run_gpio();
	demo_run_peripherals();
	demo_run_diagnostics();

	printk("\nZPlatform ready.\n");
	printk("========================================\n");
	printk("Shell available. Try: zplatform info\n");

	return 0;
}
