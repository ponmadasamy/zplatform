/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <zplatform/info.h>
#include <zplatform/diag.h>

#include "demo_internal.h"

void demo_run_diagnostics(void)
{
	struct zplatform_sys_info info;

	if (zplatform_get_sys_info(&info) == 0) {
		printk("System details\n");
		printk(" MCU board: %s\n", info.board);
		printk(" CPU frequency: %u Hz\n", info.cpu_freq_hz);
		printk(" Flash size: %u\n", info.flash_size);
		printk(" RAM size: %u\n", info.sram_size);
		printk(" ZPlatform: %s\n", info.zplatform_version);
		printk(" Zephyr: %s\n", info.zephyr_version);
		printk("\n");
	}

	zplatform_diag_report();
}
