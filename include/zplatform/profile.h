/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZPLATFORM_PROFILE_H_
#define ZPLATFORM_PROFILE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Read DWT cycle counter when available (Cortex-M3).
 *
 * Returns 0 if the cycle counter is not available or not enabled.
 */
uint32_t zplatform_profile_cycles(void);

/**
 * @brief Enable DWT CYCCNT if present. Safe no-op when unavailable.
 */
void zplatform_profile_init(void);

/**
 * @brief Measure elapsed cycles between two samples (handles wrap).
 */
uint32_t zplatform_profile_elapsed(uint32_t start, uint32_t end);

#ifdef __cplusplus
}
#endif

#endif /* ZPLATFORM_PROFILE_H_ */
