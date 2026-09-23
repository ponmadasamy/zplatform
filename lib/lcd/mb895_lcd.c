/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * STM3210E-EVAL MB895 LCD over FSMC bank4.
 * Pinmux / timing / init sequences derived from ST Cube F1 BSP
 * (STM3210E_EVAL + hx8347d / spfd5408 / ili9320 components).
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <errno.h>

#include <zplatform/mb895_lcd.h>

#include <stm32f1xx_hal.h>
#include <stm32f1xx_hal_gpio.h>
#include <stm32f1xx_hal_sram.h>
#include <stm32f1xx_hal_gpio_ex.h>

#if !defined(CONFIG_SOC_SERIES_STM32F1X)
#error "zplatform MB895 LCD requires STM32F1 FSMC"
#endif

struct lcd_io {
	volatile uint16_t reg;
	volatile uint16_t ram;
};

#define MB895_LCD ((struct lcd_io *)FSMC_BANK1_4)

#define LCD_REG_GRAM 0x22U

/* Sentinel: reg == DELAY_REG → val is milliseconds */
#define DELAY_REG 0xFFFFU

struct lcd_cmd {
	uint16_t reg;
	uint16_t val;
};

static bool fsmc_ready;
static uint16_t probed_id;
static uint8_t ctrl_kind; /* 0=hx8347d, 1=ili-family (ili9320/spfd5408/9325) */

enum {
	CTRL_HX8347D = 0,
	CTRL_ILI_FAMILY = 1,
};

static void lcd_write_reg(uint8_t reg)
{
	MB895_LCD->reg = reg;
}

static void lcd_write_data(uint16_t data)
{
	MB895_LCD->ram = data;
}

static uint16_t lcd_read_data(void)
{
	return MB895_LCD->ram;
}

static void lcd_write_cmd(uint8_t reg, uint16_t val)
{
	lcd_write_reg(reg);
	lcd_write_data(val);
}

static uint16_t lcd_read_cmd(uint8_t reg)
{
	lcd_write_reg(reg);
	return lcd_read_data();
}

static void lcd_apply_table(const struct lcd_cmd *tbl, size_t n)
{
	for (size_t i = 0; i < n; i++) {
		if (tbl[i].reg == DELAY_REG) {
			k_msleep(tbl[i].val);
		} else {
			lcd_write_cmd((uint8_t)tbl[i].reg, tbl[i].val);
		}
	}
}

static void fsmc_gpio_init(void)
{
	GPIO_InitTypeDef gpio = {0};

	__HAL_RCC_FSMC_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOE_CLK_ENABLE();
	__HAL_RCC_GPIOF_CLK_ENABLE();
	__HAL_RCC_GPIOG_CLK_ENABLE();
	__HAL_RCC_AFIO_CLK_ENABLE();

	gpio.Mode = GPIO_MODE_AF_PP;
	gpio.Speed = GPIO_SPEED_FREQ_HIGH;
	gpio.Pull = GPIO_NOPULL;

	/* PD0/1/4/5/8/9/10/14/15 — D2,D3,NOE,NWE,D13-D15,D0,D1 */
	gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5 |
		   GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_14 |
		   GPIO_PIN_15;
	HAL_GPIO_Init(GPIOD, &gpio);

	/* PE7-15 — D4-D12 */
	gpio.Pin = GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 |
		   GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 |
		   GPIO_PIN_15;
	HAL_GPIO_Init(GPIOE, &gpio);

	/* PF0 — A0 (RS) */
	gpio.Pin = GPIO_PIN_0;
	HAL_GPIO_Init(GPIOF, &gpio);

	/* PG12 — NE4 (LCD CS) */
	gpio.Pin = GPIO_PIN_12;
	HAL_GPIO_Init(GPIOG, &gpio);

	/*
	 * FSMC NADV shares PB7 with I2C1 SDA. Disconnect NADV so STLM75
	 * keeps working (Cube BSP does the same).
	 */
	__HAL_AFIO_FSMCNADV_DISCONNECTED();
}

static int fsmc_bank4_init(void)
{
	SRAM_HandleTypeDef hsram = {0};
	FSMC_NORSRAM_TimingTypeDef timing = {0};

	fsmc_gpio_init();

	hsram.Instance = FSMC_NORSRAM_DEVICE;
	hsram.Extended = FSMC_NORSRAM_EXTENDED_DEVICE;

	timing.AddressSetupTime = 1;
	timing.AddressHoldTime = 1;
	timing.DataSetupTime = 2;
	timing.BusTurnAroundDuration = 1;
	timing.CLKDivision = 2;
	timing.DataLatency = 2;
	timing.AccessMode = FSMC_ACCESS_MODE_A;

	hsram.Init.NSBank = FSMC_NORSRAM_BANK4;
	hsram.Init.DataAddressMux = FSMC_DATA_ADDRESS_MUX_DISABLE;
	hsram.Init.MemoryType = FSMC_MEMORY_TYPE_SRAM;
	hsram.Init.MemoryDataWidth = FSMC_NORSRAM_MEM_BUS_WIDTH_16;
	hsram.Init.BurstAccessMode = FSMC_BURST_ACCESS_MODE_DISABLE;
	hsram.Init.WaitSignalPolarity = FSMC_WAIT_SIGNAL_POLARITY_LOW;
	hsram.Init.WrapMode = FSMC_WRAP_MODE_DISABLE;
	hsram.Init.WaitSignalActive = FSMC_WAIT_TIMING_BEFORE_WS;
	hsram.Init.WriteOperation = FSMC_WRITE_OPERATION_ENABLE;
	hsram.Init.WaitSignal = FSMC_WAIT_SIGNAL_DISABLE;
	hsram.Init.ExtendedMode = FSMC_EXTENDED_MODE_DISABLE;
	hsram.Init.AsynchronousWait = FSMC_ASYNCHRONOUS_WAIT_DISABLE;
	hsram.Init.WriteBurst = FSMC_WRITE_BURST_DISABLE;
	hsram.Init.PageSize = FSMC_PAGE_SIZE_NONE;

	if (HAL_SRAM_Init(&hsram, &timing, &timing) != HAL_OK) {
		return -EIO;
	}

	fsmc_ready = true;
	return 0;
}

/* HX8347D init (Cube hx8347d_Init), delays included. */
static const struct lcd_cmd hx8347d_init[] = {
	{0xEA, 0x00}, {0xEB, 0x20}, {0xEC, 0x0C}, {0xED, 0xC4},
	{0xE8, 0x40}, {0xE9, 0x38}, {0xF1, 0x01}, {0xF2, 0x10},
	{0x27, 0xA3},
	{0x40, 0x01}, {0x41, 0x00}, {0x42, 0x00}, {0x43, 0x10},
	{0x44, 0x0E}, {0x45, 0x24}, {0x46, 0x04}, {0x47, 0x50},
	{0x48, 0x02}, {0x49, 0x13}, {0x4A, 0x19}, {0x4B, 0x19},
	{0x4C, 0x16}, {0x50, 0x1B}, {0x51, 0x31}, {0x52, 0x2F},
	{0x53, 0x3F}, {0x54, 0x3F}, {0x55, 0x3E}, {0x56, 0x2F},
	{0x57, 0x7B}, {0x58, 0x09}, {0x59, 0x06}, {0x5A, 0x06},
	{0x5B, 0x0C}, {0x5C, 0x1D}, {0x5D, 0xCC},
	{0x1B, 0x1B}, {0x1A, 0x01}, {0x24, 0x2F}, {0x25, 0x57},
	{0x23, 0x86},
	{0x18, 0x36}, {0x19, 0x01}, {0x1C, 0x06}, {0x1D, 0x06},
	{0x1F, 0x90}, {0x27, 0x01},
	{DELAY_REG, 10},
	{0x17, 0x05}, {0x36, 0x09},
	{0x28, 0x38},
	{DELAY_REG, 60},
	{0x28, 0x3C},
	{0x01, 0x00}, {0x02, 0x00}, {0x03, 0x00}, {0x04, 0x01},
	{0x05, 0x3F}, {0x06, 0x00}, {0x07, 0x00}, {0x08, 0x00},
	{0x09, 0xEF}, {0x16, 0xE0},
};

/* SPFD5408 init (Cube spfd5408_Init). */
static const struct lcd_cmd spfd5408_init[] = {
	{0xE3, 0x3008}, {0xE7, 0x0012}, {0xEF, 0x1231},
	{0x01, 0x0100}, {0x02, 0x0700}, {0x03, 0x1030}, {0x04, 0x0000},
	{0x08, 0x0202}, {0x09, 0x0000}, {0x0A, 0x0000}, {0x0C, 0x0000},
	{0x0D, 0x0000}, {0x0F, 0x0000},
	{0x10, 0x0000}, {0x11, 0x0000}, {0x12, 0x0000}, {0x13, 0x0000},
	{0x11, 0x0007}, {0x10, 0x12B0}, {0x12, 0x01BD}, {0x13, 0x1400},
	{0x29, 0x000E}, {0x20, 0x0000}, {0x21, 0x013F},
	{0x30, 0x0007}, {0x31, 0x0302}, {0x32, 0x0105}, {0x35, 0x0206},
	{0x36, 0x0808}, {0x37, 0x0206}, {0x38, 0x0504}, {0x39, 0x0007},
	{0x3C, 0x0105}, {0x3D, 0x0808},
	{0x50, 0x0000}, {0x51, 0x00EF}, {0x52, 0x0000}, {0x53, 0x013F},
	{0x60, 0xA700}, {0x61, 0x0001}, {0x6A, 0x0000},
	{0x80, 0x0000}, {0x81, 0x0000}, {0x82, 0x0000}, {0x83, 0x0000},
	{0x84, 0x0000}, {0x85, 0x0000},
	{0x90, 0x0010}, {0x92, 0x0000}, {0x93, 0x0003}, {0x95, 0x0110},
	{0x97, 0x0000}, {0x98, 0x0000},
	{0x03, 0x1018}, {0x07, 0x0112},
};

/* ILI9320 init (Cube ili9320_Init). */
static const struct lcd_cmd ili9320_init[] = {
	{0xE5, 0x8000}, {0x00, 0x0001}, {0x01, 0x0100}, {0x02, 0x0700},
	{0x03, 0x1030}, {0x04, 0x0000}, {0x08, 0x0202}, {0x09, 0x0000},
	{0x0A, 0x0000}, {0x0C, 0x0000}, {0x0D, 0x0000}, {0x0F, 0x0000},
	{0x10, 0x0000}, {0x11, 0x0000}, {0x12, 0x0000}, {0x13, 0x0000},
	{0x10, 0x17B0}, {0x11, 0x0137}, {0x12, 0x0139}, {0x13, 0x1d00},
	{0x29, 0x0013}, {0x20, 0x0000}, {0x21, 0x0000},
	{0x30, 0x0007}, {0x31, 0x0007}, {0x32, 0x0007}, {0x35, 0x0007},
	{0x36, 0x0007}, {0x37, 0x0700}, {0x38, 0x0700}, {0x39, 0x0700},
	{0x3C, 0x0700}, {0x3D, 0x1F00},
	{0x50, 0x0000}, {0x51, 0x00EF}, {0x52, 0x0000}, {0x53, 0x013F},
	{0x60, 0x2700}, {0x61, 0x0001}, {0x6A, 0x0000},
	{0x80, 0x0000}, {0x81, 0x0000}, {0x82, 0x0000}, {0x83, 0x0000},
	{0x84, 0x0000}, {0x85, 0x0000},
	{0x90, 0x0010}, {0x92, 0x0000}, {0x93, 0x0003}, {0x95, 0x0110},
	{0x97, 0x0000}, {0x98, 0x0000},
	{0x03, 0x1018}, {0x07, 0x0173},
};

uint16_t zplatform_mb895_lcd_id(void)
{
	return probed_id;
}

const char *zplatform_mb895_lcd_id_name(uint16_t id)
{
	switch (id) {
	case ZPLATFORM_MB895_ID_HX8347D:
		return "HX8347D";
	case ZPLATFORM_MB895_ID_SPFD5408:
		return "SPFD5408";
	case ZPLATFORM_MB895_ID_ILI9320:
		return "ILI9320";
	case ZPLATFORM_MB895_ID_ILI9325:
		return "ILI9325";
	default:
		return "unknown";
	}
}

int zplatform_mb895_lcd_init(uint16_t *id_out)
{
	uint16_t id;
	int rc;

	if (!fsmc_ready) {
		rc = fsmc_bank4_init();
		if (rc < 0) {
			return rc;
		}
	}

	/* Raw ID — do not run controller Init before probe. */
	id = lcd_read_cmd(0x00);
	probed_id = id;
	if (id_out) {
		*id_out = id;
	}

	switch (id) {
	case ZPLATFORM_MB895_ID_HX8347D:
		ctrl_kind = CTRL_HX8347D;
		lcd_apply_table(hx8347d_init, ARRAY_SIZE(hx8347d_init));
		break;
	case ZPLATFORM_MB895_ID_SPFD5408:
		ctrl_kind = CTRL_ILI_FAMILY;
		lcd_apply_table(spfd5408_init, ARRAY_SIZE(spfd5408_init));
		break;
	case ZPLATFORM_MB895_ID_ILI9320:
	case ZPLATFORM_MB895_ID_ILI9325:
		ctrl_kind = CTRL_ILI_FAMILY;
		lcd_apply_table(ili9320_init, ARRAY_SIZE(ili9320_init));
		break;
	default:
		return -ENODEV;
	}

	return zplatform_mb895_lcd_set_window(0, 0, ZPLATFORM_MB895_LCD_WIDTH,
					      ZPLATFORM_MB895_LCD_HEIGHT);
}

/*
 * HX8347D (Cube SetDisplayWindow) with MAC 0xE0 (MV=1):
 * Cube API is (Xpos, Ypos, Height, Width) where Xpos→row regs and
 * Ypos→column regs. Zephyr landscape (x,y,w,h) with 320×240 maps as
 * Cube(Xpos=y, Ypos=x, Height=h, Width=w) so full frame uses
 * rows 0..239 / cols 0..319 — matching the Cube init window.
 */
static int set_window_hx8347d(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
	const uint16_t row_s = y;
	const uint16_t row_e = (uint16_t)(y + h - 1U);
	const uint16_t col_s = x;
	const uint16_t col_e = (uint16_t)(x + w - 1U);

	lcd_write_cmd(0x06, (uint16_t)(row_s >> 8));
	lcd_write_cmd(0x07, (uint16_t)(row_s & 0xFF));
	lcd_write_cmd(0x08, (uint16_t)(row_e >> 8));
	lcd_write_cmd(0x09, (uint16_t)(row_e & 0xFF));
	lcd_write_cmd(0x02, (uint16_t)(col_s >> 8));
	lcd_write_cmd(0x03, (uint16_t)(col_s & 0xFF));
	lcd_write_cmd(0x04, (uint16_t)(col_e >> 8));
	lcd_write_cmd(0x05, (uint16_t)(col_e & 0xFF));
	lcd_write_reg(LCD_REG_GRAM);
	return 0;
}

static int set_window_ili(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
	lcd_write_cmd(0x50, y);
	lcd_write_cmd(0x51, (uint16_t)(y + h - 1U));
	lcd_write_cmd(0x52, x);
	lcd_write_cmd(0x53, (uint16_t)(x + w - 1U));
	lcd_write_cmd(0x20, y);
	lcd_write_cmd(0x21, (uint16_t)(ZPLATFORM_MB895_LCD_WIDTH - 1U - x));
	lcd_write_reg(LCD_REG_GRAM);
	return 0;
}

int zplatform_mb895_lcd_set_window(uint16_t x, uint16_t y, uint16_t w,
				   uint16_t h)
{
	if (!fsmc_ready || w == 0U || h == 0U) {
		return -EINVAL;
	}
	if ((uint32_t)x + w > ZPLATFORM_MB895_LCD_WIDTH ||
	    (uint32_t)y + h > ZPLATFORM_MB895_LCD_HEIGHT) {
		return -EINVAL;
	}

	if (ctrl_kind == CTRL_HX8347D) {
		return set_window_hx8347d(x, y, w, h);
	}
	return set_window_ili(x, y, w, h);
}

void zplatform_mb895_lcd_write_pixels(const uint16_t *pixels, size_t count)
{
	for (size_t i = 0; i < count; i++) {
		lcd_write_data(pixels[i]);
	}
}

int zplatform_mb895_lcd_write_rect(uint16_t x, uint16_t y, uint16_t w,
				   uint16_t h, uint16_t pitch,
				   const uint16_t *buf)
{
	int rc;

	if (buf == NULL || pitch < w) {
		return -EINVAL;
	}

	rc = zplatform_mb895_lcd_set_window(x, y, w, h);
	if (rc < 0) {
		return rc;
	}

	if (pitch == w) {
		zplatform_mb895_lcd_write_pixels(buf, (size_t)w * h);
		return 0;
	}

	for (uint16_t row = 0; row < h; row++) {
		zplatform_mb895_lcd_write_pixels(&buf[(size_t)row * pitch], w);
	}
	return 0;
}

int zplatform_mb895_lcd_fill(uint16_t rgb565)
{
	const uint32_t pixels =
		(uint32_t)ZPLATFORM_MB895_LCD_WIDTH * ZPLATFORM_MB895_LCD_HEIGHT;
	int rc;

	rc = zplatform_mb895_lcd_set_window(0, 0, ZPLATFORM_MB895_LCD_WIDTH,
					    ZPLATFORM_MB895_LCD_HEIGHT);
	if (rc < 0) {
		return rc;
	}

	for (uint32_t i = 0; i < pixels; i++) {
		lcd_write_data(rgb565);
	}
	return 0;
}

int zplatform_mb895_lcd_blanking_on(void)
{
	if (!fsmc_ready) {
		return -ENODEV;
	}

	if (ctrl_kind == CTRL_HX8347D) {
		lcd_write_cmd(0x28, 0x38);
		k_msleep(40);
		lcd_write_cmd(0x28, 0x04);
		return 0;
	}

	/* ILI / SPFD: display OFF */
	lcd_write_cmd(0x07, 0x0000);
	return 0;
}

int zplatform_mb895_lcd_blanking_off(void)
{
	if (!fsmc_ready) {
		return -ENODEV;
	}

	if (ctrl_kind == CTRL_HX8347D) {
		lcd_write_cmd(0x28, 0x38);
		k_msleep(40);
		lcd_write_cmd(0x28, 0x3C);
		return 0;
	}

	if (probed_id == ZPLATFORM_MB895_ID_SPFD5408) {
		lcd_write_cmd(0x07, 0x0112);
	} else {
		lcd_write_cmd(0x07, 0x0173);
	}
	return 0;
}
