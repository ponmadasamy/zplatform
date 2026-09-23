/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * STM3210E-EVAL integrated demo: console smoke + LVGL UI.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/version.h>

#if defined(CONFIG_BOOTLOADER_MCUBOOT)
#include <zephyr/dfu/mcuboot.h>
#endif

#include <zplatform/info.h>
#include <zplatform/brand.h>

#include "demo_internal.h"

int main(void)
{
	struct zplatform_sys_info info;

	printk("========================================\n");
	printk(" %s\n", ZPLATFORM_BRAND_NAME);
	printk(" %s\n", ZPLATFORM_BRAND_TAGLINE);
	if (zplatform_get_sys_info(&info) == 0) {
		printk(" %s\n", info.board);
	}
	printk(" %s\n", ZPLATFORM_BRAND_IMPACT);
	printk("========================================\n\n");

	printk("System\n");
	printk(" [PASS] Boot\n");
	printk(" [PASS] Clock (%u Hz)\n",
	       info.cpu_freq_hz ? info.cpu_freq_hz :
				  sys_clock_hw_cycles_per_sec());
	printk(" [PASS] Kernel (%s)\n", KERNEL_VERSION_STRING);
	printk(" [PASS] Console\n");
#if defined(CONFIG_BOOTLOADER_MCUBOOT)
	if (!boot_is_img_confirmed()) {
		int cret = boot_write_img_confirmed();

		if (cret == 0) {
			printk(" [PASS] MCUboot image confirmed\n");
		} else {
			printk(" [FAIL] MCUboot confirm (%d)\n", cret);
		}
	} else {
		printk(" [PASS] MCUboot image already confirmed\n");
	}
#endif
	printk("\n");

	demo_run_gpio();
	demo_run_peripherals();
	demo_run_diagnostics();

	printk("\n%s ready.\n", ZPLATFORM_BRAND_NAME);
	printk("%s\n", ZPLATFORM_BRAND_VISION);
	printk("========================================\n");
	printk("Shell available. Try: zplatform info\n");
	printk("LCD UI: Joy Left/Right change screens\n");

#if defined(CONFIG_LVGL) && defined(CONFIG_BOARD_STM3210E_EVAL)
	demo_ui_run();
#endif

	return 0;
}
