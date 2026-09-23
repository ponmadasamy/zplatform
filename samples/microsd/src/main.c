/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * STM3210E-EVAL microSD on CN13 (SDIO).
 * Jumpers: JP17 fitted; JP20 open (SWD); JP22 open for 4-bit D3.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <string.h>

#include <zplatform/sdio_sd.h>

static uint8_t block_buf[ZPLATFORM_SD_BLOCK_SIZE] __aligned(4);
static uint8_t save_buf[ZPLATFORM_SD_BLOCK_SIZE] __aligned(4);

int main(void)
{
	struct zplatform_sd_info info;
	uint32_t test_lba;
	int rc;

	printk("microSD sample\n");

#if !defined(CONFIG_ZPLATFORM_SDIO_SD)
	printk("[SKIP] CONFIG_ZPLATFORM_SDIO_SD not enabled\n");
	return 0;
#else
	if (!zplatform_sdio_sd_detected()) {
		printk("[SKIP] microSD not detected (PF11); insert card\n");
		return 0;
	}
	printk("[PASS] microSD detect\n");

	rc = zplatform_sdio_sd_init();
	if (rc != 0) {
		printk("[FAIL] microSD init (%d)\n", rc);
		return 0;
	}
	printk("[PASS] microSD init\n");

	rc = zplatform_sdio_sd_get_info(&info);
	if (rc != 0) {
		printk("[FAIL] microSD get_info (%d)\n", rc);
		return 0;
	}
	printk("[PASS] microSD blocks=%u size=%u type=%u\n",
	       info.log_block_nbr, info.log_block_size, info.card_type);

	rc = zplatform_sdio_sd_read(0, block_buf, 1);
	if (rc != 0) {
		printk("[FAIL] microSD read LBA0 (%d)\n", rc);
		return 0;
	}
	printk("[PASS] microSD read LBA0: %02x %02x %02x %02x\n",
	       block_buf[0], block_buf[1], block_buf[2], block_buf[3]);

	if (info.log_block_nbr < 2U) {
		printk("[SKIP] microSD R/W selftest (card too small)\n");
		return 0;
	}

	/* Use last logical block; save/restore contents. */
	test_lba = info.log_block_nbr - 1U;
	rc = zplatform_sdio_sd_read(test_lba, save_buf, 1);
	if (rc != 0) {
		printk("[FAIL] microSD read LBA%u (%d)\n", test_lba, rc);
		return 0;
	}

	memset(block_buf, 0x5A, sizeof(block_buf));
	block_buf[0] = 'Z';
	block_buf[1] = 'P';
	block_buf[2] = 'S';
	block_buf[3] = 'D';

	rc = zplatform_sdio_sd_write(test_lba, block_buf, 1);
	if (rc != 0) {
		printk("[FAIL] microSD write LBA%u (%d)\n", test_lba, rc);
		return 0;
	}

	memset(block_buf, 0, sizeof(block_buf));
	rc = zplatform_sdio_sd_read(test_lba, block_buf, 1);
	if (rc != 0) {
		printk("[FAIL] microSD verify-read LBA%u (%d)\n", test_lba, rc);
		(void)zplatform_sdio_sd_write(test_lba, save_buf, 1);
		return 0;
	}

	if (memcmp(block_buf, "ZPSD", 4) != 0 || block_buf[4] != 0x5A) {
		printk("[FAIL] microSD verify mismatch\n");
		(void)zplatform_sdio_sd_write(test_lba, save_buf, 1);
		return 0;
	}

	rc = zplatform_sdio_sd_write(test_lba, save_buf, 1);
	if (rc != 0) {
		printk("[FAIL] microSD restore LBA%u (%d)\n", test_lba, rc);
		return 0;
	}

	printk("[PASS] microSD R/W selftest LBA%u\n", test_lba);
	return 0;
#endif
}
