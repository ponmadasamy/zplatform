/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Linux-inspired LED triggers for GPIO LEDs (Zephyr has no
 * linux,default-trigger). Patterns are software-driven.
 */

#ifndef ZPLATFORM_LED_TRIGGERS_H_
#define ZPLATFORM_LED_TRIGGERS_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start LED trigger service for board LEDs.
 *
 * Default mapping (when aliases exist):
 * - led0: heartbeat
 * - led1: cpu
 * - led2: timer
 * - led3: panic (off until fatal error)
 *
 * @retval 0 on success
 * @retval -ENODEV if no usable LED aliases
 */
int zplatform_led_triggers_start(void);

/** Latch panic trigger LEDs on (fatal-error path). */
void zplatform_led_triggers_on_panic(void);

/** Pause or resume pattern updates (LEDs freeze in current state when paused). */
void zplatform_led_triggers_pause(bool pause);

/** @return true if pattern updates are paused */
bool zplatform_led_triggers_paused(void);

/**
 * @brief Rotate trigger assignments across LEDs.
 *
 * @param dir +1 = right (toward higher LED index, last wraps to first);
 *            -1 = left (first wraps to last)
 */
void zplatform_led_triggers_rotate(int dir);

/**
 * @brief Adjust timer trigger pulse gap.
 *
 * TIM pattern is a fixed ~200 ms flash, then @p gap off. This adjusts the gap.
 *
 * @param delta_ms Added to gap (clamped).
 * @return New gap in ms.
 */
uint32_t zplatform_led_triggers_timer_adjust(int delta_ms);

/** Current timer gap (ms) between TIM pulses. */
uint32_t zplatform_led_triggers_timer_ms(void);

/**
 * @brief Format current LD1.. map into @p buf (e.g. "HB CPU TIM PAN").
 * @return bytes written (excluding NUL), or negative errno
 */
int zplatform_led_triggers_format_map(char *buf, size_t buflen);

#ifdef __cplusplus
}
#endif

#endif /* ZPLATFORM_LED_TRIGGERS_H_ */
