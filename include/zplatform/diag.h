/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZPLATFORM_DIAG_H_
#define ZPLATFORM_DIAG_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct zplatform_mem_info {
	size_t sram_total;
	size_t sram_used;
	size_t sram_free;
	size_t flash_total;
	size_t flash_used;
};

struct zplatform_thread_stats {
	uint32_t thread_count;
};

/**
 * @brief Collect memory usage estimates from Zephyr APIs.
 */
int zplatform_get_mem_info(struct zplatform_mem_info *info);

/**
 * @brief Count currently known threads where supported.
 */
int zplatform_get_thread_stats(struct zplatform_thread_stats *stats);

/**
 * @brief Log memory / thread diagnostics.
 */
void zplatform_diag_report(void);

#ifdef __cplusplus
}
#endif

#endif /* ZPLATFORM_DIAG_H_ */
