/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/devicetree.h>
#include <zephyr/version.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <zephyr/drivers/hwinfo.h>

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
	info->build_date = __DATE__;
	info->build_time = __TIME__;
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

#if defined(CONFIG_HWINFO)
	{
		ssize_t n = hwinfo_get_device_id(info->uid, sizeof(info->uid));

		if (n > 0) {
			info->uid_len = (uint8_t)MIN((size_t)n, sizeof(info->uid));
		}

		(void)hwinfo_get_reset_cause(&info->reset_cause);
	}
#endif

	return 0;
}

int zplatform_format_uid(const struct zplatform_sys_info *info, char *buf, size_t len)
{
	size_t need;
	size_t i;

	if (info == NULL || buf == NULL || len == 0) {
		return -EINVAL;
	}

	if (info->uid_len == 0) {
		if (len < 2) {
			return -ENOMEM;
		}
		buf[0] = '-';
		buf[1] = '\0';
		return 1;
	}

	need = (size_t)info->uid_len * 2U + 1U;
	if (len < need) {
		return -ENOMEM;
	}

	for (i = 0; i < info->uid_len; i++) {
		(void)snprintf(&buf[i * 2U], 3, "%02X", info->uid[i]);
	}

	return (int)(info->uid_len * 2U);
}

int zplatform_format_reset_cause(uint32_t cause, char *buf, size_t len)
{
	static const struct {
		uint32_t bit;
		const char *name;
	} causes[] = {
		{ RESET_PIN, "pin" },
		{ RESET_SOFTWARE, "software" },
		{ RESET_BROWNOUT, "brownout" },
		{ RESET_POR, "por" },
		{ RESET_WATCHDOG, "watchdog" },
		{ RESET_DEBUG, "debug" },
		{ RESET_SECURITY, "security" },
		{ RESET_LOW_POWER_WAKE, "low-power" },
		{ RESET_CPU_LOCKUP, "lockup" },
		{ RESET_PARITY, "parity" },
		{ RESET_PLL, "pll" },
		{ RESET_CLOCK, "clock" },
		{ RESET_HARDWARE, "hardware" },
		{ RESET_USER, "user" },
		{ RESET_TEMPERATURE, "temperature" },
		{ RESET_BOOTLOADER, "bootloader" },
		{ RESET_FLASH, "flash" },
	};
	size_t used = 0;
	bool first = true;
	size_t i;

	if (buf == NULL || len == 0) {
		return -EINVAL;
	}

	buf[0] = '\0';

	if (cause == 0) {
		if (len < 5) {
			return -ENOMEM;
		}
		memcpy(buf, "none", 5);
		return 4;
	}

	for (i = 0; i < ARRAY_SIZE(causes); i++) {
		size_t nlen;
		size_t need;

		if ((cause & causes[i].bit) == 0U) {
			continue;
		}

		nlen = strlen(causes[i].name);
		need = nlen + (first ? 0U : 2U);
		if (used + need + 1U > len) {
			return -ENOMEM;
		}

		if (!first) {
			buf[used++] = ',';
			buf[used++] = ' ';
		}
		memcpy(&buf[used], causes[i].name, nlen);
		used += nlen;
		first = false;
	}

	if (first) {
		int n = snprintf(buf, len, "0x%08x", cause);

		return (n < 0 || (size_t)n >= len) ? -ENOMEM : n;
	}

	buf[used] = '\0';
	return (int)used;
}

void zplatform_print_banner(void)
{
	struct zplatform_sys_info info;
	char uid[ZPLATFORM_UID_MAX_LEN * 2 + 1];
	char reset[96];

	if (zplatform_get_sys_info(&info) != 0) {
		printk("ZPlatform: unable to read system info\n");
		return;
	}

	(void)zplatform_format_uid(&info, uid, sizeof(uid));
	(void)zplatform_format_reset_cause(info.reset_cause, reset, sizeof(reset));

	printk("========================================\n");
	printk(" ZPlatform %s\n", info.zplatform_version);
	printk(" Board: %s\n", info.board);
	printk(" Zephyr: %s\n", info.zephyr_version);
	printk(" Build: %s %s\n", info.build_date, info.build_time);
	printk(" UID: %s\n", uid);
	printk(" Reset: %s\n", reset);
	printk(" CPU: %u Hz\n", info.cpu_freq_hz);
	printk(" Flash: %u bytes\n", info.flash_size);
	printk(" SRAM: %u bytes\n", info.sram_size);
	printk("========================================\n");
}
