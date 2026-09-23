/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * USB FS CDC ACM on CN14 (UM0488 §2.15). Keep CAN disabled (shared IRQs).
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/sys/printk.h>

int main(void)
{
	const struct device *cdc = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);
	uint32_t dtr = 0;
	int rc;

	printk("USB CDC sample\n");

	if (!device_is_ready(cdc)) {
		printk("[FAIL] CDC ACM not ready\n");
		return 0;
	}

	rc = usb_enable(NULL);
	if (rc != 0 && rc != -EALREADY) {
		printk("[FAIL] USB enable (%d)\n", rc);
		return 0;
	}

	printk("[PASS] USB CDC ACM (CN14)\n");

	/* Wait for host terminal DTR (optional). */
	while (true) {
		(void)uart_line_ctrl_get(cdc, UART_LINE_CTRL_DTR, &dtr);
		if (dtr) {
			break;
		}
		k_msleep(100);
	}

	printk("Host connected — echo on CDC\n");

	while (1) {
		uint8_t c;

		if (uart_poll_in(cdc, &c) == 0) {
			uart_poll_out(cdc, c);
		}
		k_msleep(5);
	}

	return 0;
}
