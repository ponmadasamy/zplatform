/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Board-agnostic SPI NOR sample. Uses the first jedec,spi-nor node when
 * present (M25P64 on STM3210E-EVAL).
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#if DT_HAS_COMPAT_STATUS_OKAY(jedec_spi_nor)
#define FLASH_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(jedec_spi_nor)
#else
#define FLASH_NODE DT_INVALID_NODE
#endif

int main(void)
{
	printk("SPI flash sample\n");

#if !DT_NODE_EXISTS(FLASH_NODE)
	printk("[SKIP] No jedec,spi-nor device in DeviceTree\n");
	return 0;
#else
	const struct device *flash = DEVICE_DT_GET(FLASH_NODE);
	uint8_t id[3] = {0};
	uint8_t buf[16];
	int rc;

	if (!device_is_ready(flash)) {
		printk("[FAIL] SPI NOR device not ready\n");
		return 0;
	}

	printk("[PASS] SPI NOR ready (%s)\n", flash->name);

	rc = flash_read_jedec_id(flash, id);
	if (rc != 0) {
		printk("[FAIL] JEDEC read-ID (%d)\n", rc);
		return 0;
	}

	printk("[PASS] JEDEC ID %02x %02x %02x\n", id[0], id[1], id[2]);

	rc = flash_read(flash, 0, buf, sizeof(buf));
	if (rc != 0) {
		printk("[FAIL] flash_read @0 (%d)\n", rc);
		return 0;
	}

	printk("[PASS] read 16 B @0:");
	for (size_t i = 0; i < sizeof(buf); i++) {
		printk(" %02x", buf[i]);
	}
	printk("\n");

	return 0;
#endif
}
