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
	char uid[(ZPLATFORM_UID_MAX_LEN * 2) + 1];
	char reset[96];

	if (zplatform_get_sys_info(&info) == 0) {
		(void)zplatform_format_uid(&info, uid, sizeof(uid));
		(void)zplatform_format_reset_cause(info.reset_cause, reset, sizeof(reset));

		printk("System details\n");
		printk(" MCU board: %s\n", info.board);
		printk(" Build: %s %s\n", info.build_date, info.build_time);
		printk(" UID: %s\n", uid);
		printk(" Reset: %s\n", reset);
		printk(" CPU frequency: %u Hz\n", info.cpu_freq_hz);
		printk(" Flash size: %u\n", info.flash_size);
		printk(" RAM size: %u\n", info.sram_size);
		printk(" ZPlatform: %s\n", info.zplatform_version);
		printk(" Zephyr: %s\n", info.zephyr_version);
		printk("\n");
	}

	zplatform_diag_report();
}
