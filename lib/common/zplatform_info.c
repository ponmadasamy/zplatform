/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/devicetree.h>
#include <zephyr/version.h>
#include <string.h>

#include <zplatform/info.h>
#include <zplatform/version.h>

int zplatform_get_sys_info(struct zplatform_sys_info *info)
{
	if (info == NULL) {
		return -EINVAL;
	}

	memset(info, 0, sizeof(*info));
	info->zplatform_version = ZPLATFORM_VERSION_STRING;
	info->zephyr_version = KERNEL_VERSION_STRING;
	info->board = CONFIG_BOARD;
	info->cpu_freq_hz = sys_clock_hw_cycles_per_sec();

#if DT_NODE_EXISTS(DT_CHOSEN(zephyr_flash))
	info->flash_size = DT_REG_SIZE(DT_CHOSEN(zephyr_flash));
#elif DT_NODE_EXISTS(DT_NODELABEL(flash0))
	info->flash_size = DT_REG_SIZE(DT_NODELABEL(flash0));
#endif

#if DT_NODE_EXISTS(DT_CHOSEN(zephyr_sram))
	info->sram_size = DT_REG_SIZE(DT_CHOSEN(zephyr_sram));
#elif DT_NODE_EXISTS(DT_NODELABEL(sram0))
	info->sram_size = DT_REG_SIZE(DT_NODELABEL(sram0));
#endif

	return 0;
}

void zplatform_print_banner(void)
{
	struct zplatform_sys_info info;

	if (zplatform_get_sys_info(&info) != 0) {
		printk("ZPlatform: unable to read system info\n");
		return;
	}

	printk("========================================\n");
	printk(" ZPlatform %s\n", info.zplatform_version);
	printk(" Board: %s\n", info.board);
	printk(" Zephyr: %s\n", info.zephyr_version);
	printk(" CPU: %u Hz\n", info.cpu_freq_hz);
	printk(" Flash: %u bytes\n", info.flash_size);
	printk(" SRAM: %u bytes\n", info.sram_size);
	printk("========================================\n");
}
