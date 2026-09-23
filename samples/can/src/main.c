/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * bxCAN loopback on STM3210E-EVAL (UM0488 §2.7, SN65HVD230 / CN4).
 * No external bus required in LOOPBACK mode.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/sys/printk.h>

#define MSG_ID 0x123

CAN_MSGQ_DEFINE(rx_msgq, 4);

int main(void)
{
	const struct device *can = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
	struct can_frame tx = {0};
	struct can_frame rx = {0};
	const struct can_filter filter = {
		.flags = 0,
		.id = MSG_ID,
		.mask = CAN_STD_ID_MASK,
	};
	int filter_id;
	int rc;

	printk("CAN sample\n");

	if (!device_is_ready(can)) {
		printk("[FAIL] CAN not ready\n");
		return 0;
	}

	rc = can_set_mode(can, CAN_MODE_LOOPBACK);
	if (rc != 0) {
		printk("[FAIL] CAN mode (%d)\n", rc);
		return 0;
	}

	rc = can_start(can);
	if (rc != 0) {
		printk("[FAIL] CAN start (%d)\n", rc);
		return 0;
	}

	filter_id = can_add_rx_filter_msgq(can, &rx_msgq, &filter);
	if (filter_id < 0) {
		printk("[FAIL] CAN filter (%d)\n", filter_id);
		return 0;
	}

	tx.id = MSG_ID;
	tx.dlc = 2;
	tx.data[0] = 0xA5;
	tx.data[1] = 0x5A;

	rc = can_send(can, &tx, K_MSEC(100), NULL, NULL);
	if (rc != 0) {
		printk("[FAIL] CAN send (%d)\n", rc);
		return 0;
	}

	rc = k_msgq_get(&rx_msgq, &rx, K_MSEC(100));
	if (rc != 0) {
		printk("[FAIL] CAN RX timeout\n");
		return 0;
	}

	if (rx.id != MSG_ID || rx.dlc != 2 || rx.data[0] != 0xA5 ||
	    rx.data[1] != 0x5A) {
		printk("[FAIL] CAN RX mismatch\n");
		return 0;
	}

	printk("[PASS] CAN loopback id=0x%X data=%02X %02X\n",
	       rx.id, rx.data[0], rx.data[1]);

	while (1) {
		static uint8_t n;

		tx.data[0] = n++;
		tx.data[1] = (uint8_t)~tx.data[0];
		(void)can_send(can, &tx, K_MSEC(50), NULL, NULL);
		if (k_msgq_get(&rx_msgq, &rx, K_MSEC(50)) == 0) {
			printk("loop %02X %02X\n", rx.data[0], rx.data[1]);
		}
		k_msleep(500);
	}

	return 0;
}
