/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * microSD on STM3210E-EVAL CN13 via SDIO (HAL_SD polling).
 * Jumpers: JP17 fitted; JP20 open (SWD); JP22 open only if using 4-bit.
 */

#ifndef ZPLATFORM_SDIO_SD_H_
#define ZPLATFORM_SDIO_SD_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ZPLATFORM_SD_BLOCK_SIZE 512U

struct zplatform_sd_info {
	uint32_t block_nbr;
	uint32_t block_size;
	uint32_t log_block_nbr;
	uint32_t log_block_size;
	uint32_t card_type;
	uint32_t card_version;
	uint32_t class;
	uint32_t rel_card_add;
};

/** Card detect on PF11 (active low). Enables GPIO if needed. */
bool zplatform_sdio_sd_detected(void);

/**
 * Init SDIO + card (4-bit bus). Returns 0, -ENODEV if no card,
 * -EIO on HAL failure.
 */
int zplatform_sdio_sd_init(void);

bool zplatform_sdio_sd_ready(void);

int zplatform_sdio_sd_get_info(struct zplatform_sd_info *info);

/** Polling block I/O. @p block is LBA; @p count is number of 512 B blocks. */
int zplatform_sdio_sd_read(uint32_t block, void *buf, uint32_t count);
int zplatform_sdio_sd_write(uint32_t block, const void *buf, uint32_t count);

#ifdef __cplusplus
}
#endif

#endif /* ZPLATFORM_SDIO_SD_H_ */
