/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * IS61WV51216BLL external SRAM on STM3210E-EVAL (FSMC bank3 @ 0x68000000).
 */

#ifndef ZPLATFORM_FSMC_SRAM_H_
#define ZPLATFORM_FSMC_SRAM_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** 512K×16 = 1 MiB */
#define ZPLATFORM_FSMC_SRAM_SIZE   0x00100000U
#define ZPLATFORM_FSMC_SRAM_BASE   0x68000000U

/** Configure FSMC bank3 + GPIO. Idempotent. */
int zplatform_fsmc_sram_init(void);

/** True after a successful init. */
bool zplatform_fsmc_sram_ready(void);

/** Mapped base pointer (NULL if not ready). */
volatile uint16_t *zplatform_fsmc_sram_base(void);

/**
 * Walking 16-bit pattern test over @p nbytes (rounded down to halfwords).
 * Returns 0 on success, -EIO on mismatch, -EINVAL if not ready / bad size.
 */
int zplatform_fsmc_sram_selftest(size_t nbytes);

#ifdef __cplusplus
}
#endif

#endif /* ZPLATFORM_FSMC_SRAM_H_ */
