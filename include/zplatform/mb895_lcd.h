/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * MB895 TFT private HAL (FSMC bank4) + Zephyr display helpers.
 * Controllers: HX8347D / SPFD5408 / ILI9320 (ST Cube BSP sequences).
 */

#ifndef ZPLATFORM_MB895_LCD_H_
#define ZPLATFORM_MB895_LCD_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Logical size matches Cube BSP + HX8347D MAC 0xE0 (320×240 landscape). */
#define ZPLATFORM_MB895_LCD_WIDTH  320U
#define ZPLATFORM_MB895_LCD_HEIGHT 240U

#define ZPLATFORM_MB895_ID_HX8347D  0x0047U
#define ZPLATFORM_MB895_ID_SPFD5408 0x5408U
#define ZPLATFORM_MB895_ID_ILI9320  0x9320U
#define ZPLATFORM_MB895_ID_ILI9325  0x9325U

#define ZPLATFORM_RGB565(r, g, b) \
	((uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | (((b) & 0xF8) >> 3)))

/** Init FSMC + probe ID + controller init. Optional @p id_out. */
int zplatform_mb895_lcd_init(uint16_t *id_out);

/** Probed ID after successful init (0 if not inited). */
uint16_t zplatform_mb895_lcd_id(void);

const char *zplatform_mb895_lcd_id_name(uint16_t id);

/**
 * Set GRAM window then select GRAM write register.
 * @p x / @p y are Zephyr display coordinates (top-left).
 */
int zplatform_mb895_lcd_set_window(uint16_t x, uint16_t y,
				   uint16_t w, uint16_t h);

/** Write @p count RGB565 pixels to the current GRAM window. */
void zplatform_mb895_lcd_write_pixels(const uint16_t *pixels, size_t count);

/**
 * Write a rectangle from a pitched RGB565 buffer (display_write helper).
 * Buffer layout: row-major, @p pitch pixels between rows.
 */
int zplatform_mb895_lcd_write_rect(uint16_t x, uint16_t y, uint16_t w,
				   uint16_t h, uint16_t pitch,
				   const uint16_t *buf);

/** Solid-fill full panel. */
int zplatform_mb895_lcd_fill(uint16_t rgb565);

/** Display blanking (panel on/off). */
int zplatform_mb895_lcd_blanking_on(void);
int zplatform_mb895_lcd_blanking_off(void);

#ifdef __cplusplus
}
#endif

#endif /* ZPLATFORM_MB895_LCD_H_ */
