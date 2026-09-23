/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * STM3210E-EVAL IS61WV51216BLL via FSMC bank3.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <zplatform/fsmc_sram.h>

/* Exercise a slice first, then a larger window (still quick on 72 MHz). */
#define TEST_BYTES_QUICK  (4U * 1024U)
#define TEST_BYTES_WIDE   (64U * 1024U)

int main(void)
{
	int rc;

	printk("FSMC SRAM sample\n");

#if !defined(CONFIG_ZPLATFORM_FSMC_SRAM)
	printk("[SKIP] CONFIG_ZPLATFORM_FSMC_SRAM not enabled\n");
	return 0;
#else
	rc = zplatform_fsmc_sram_init();
	if (rc != 0) {
		printk("[FAIL] FSMC SRAM init (%d)\n", rc);
		return 0;
	}
	printk("[PASS] FSMC SRAM init @ 0x%08x (%u KiB)\n",
	       ZPLATFORM_FSMC_SRAM_BASE, ZPLATFORM_FSMC_SRAM_SIZE / 1024U);

	rc = zplatform_fsmc_sram_selftest(TEST_BYTES_QUICK);
	if (rc != 0) {
		printk("[FAIL] FSMC SRAM selftest %u B (%d)\n", TEST_BYTES_QUICK,
		       rc);
		return 0;
	}
	printk("[PASS] FSMC SRAM selftest %u B\n", TEST_BYTES_QUICK);

	rc = zplatform_fsmc_sram_selftest(TEST_BYTES_WIDE);
	if (rc != 0) {
		printk("[FAIL] FSMC SRAM selftest %u B (%d)\n", TEST_BYTES_WIDE,
		       rc);
		return 0;
	}
	printk("[PASS] FSMC SRAM selftest %u B\n", TEST_BYTES_WIDE);
	return 0;
#endif
}
