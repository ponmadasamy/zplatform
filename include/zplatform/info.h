/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZPLATFORM_INFO_H_
#define ZPLATFORM_INFO_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** STM32 UID is 96-bit (12 bytes); leave headroom for other SoCs. */
#define ZPLATFORM_UID_MAX_LEN 16

struct zplatform_sys_info {
	const char *zplatform_version;
	const char *zephyr_version;
	const char *board;
	/** Compile-time stamp of the ZPlatform info translation unit. */
	const char *build_date;
	const char *build_time;
	uint32_t cpu_freq_hz;
	uint32_t flash_size;
	uint32_t sram_size;
	/** Zephyr hwinfo reset-cause bitmask (RESET_* flags). */
	uint32_t reset_cause;
	uint8_t uid[ZPLATFORM_UID_MAX_LEN];
	uint8_t uid_len;
};

/**
 * @brief Fill system identity information from Zephyr / DeviceTree / hwinfo.
 *
 * @param info Output structure
 * @return 0 on success, negative errno on failure
 */
int zplatform_get_sys_info(struct zplatform_sys_info *info);

/**
 * @brief Format device UID as uppercase hex (no separators).
 *
 * @return Characters written (excluding NUL), or negative errno
 */
int zplatform_format_uid(const struct zplatform_sys_info *info, char *buf, size_t len);

/**
 * @brief Format reset-cause bitmask as comma-separated names.
 *
 * @return Characters written (excluding NUL), or negative errno
 */
int zplatform_format_reset_cause(uint32_t cause, char *buf, size_t len);

/**
 * @brief Print a short platform banner to the console logger.
 */
void zplatform_print_banner(void);

#ifdef __cplusplus
}
#endif

#endif /* ZPLATFORM_INFO_H_ */
