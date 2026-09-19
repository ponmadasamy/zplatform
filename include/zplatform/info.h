/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZPLATFORM_INFO_H_
#define ZPLATFORM_INFO_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct zplatform_sys_info {
	const char *zplatform_version;
	const char *zephyr_version;
	const char *board;
	uint32_t cpu_freq_hz;
	uint32_t flash_size;
	uint32_t sram_size;
};

/**
 * @brief Fill system identity information from Zephyr / DeviceTree.
 *
 * @param info Output structure
 * @return 0 on success, negative errno on failure
 */
int zplatform_get_sys_info(struct zplatform_sys_info *info);

/**
 * @brief Print a short platform banner to the console logger.
 */
void zplatform_print_banner(void);

#ifdef __cplusplus
}
#endif

#endif /* ZPLATFORM_INFO_H_ */
