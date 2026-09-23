/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * STM3210E-EVAL IS61WV51216BLL on FSMC bank1 NOR/PSRAM3.
 * Pinmux / timing from ST Cube F1 BSP (stm3210e_eval_sram.c).
 */

#include <zephyr/kernel.h>
#include <errno.h>
#include <stdbool.h>

#include <zplatform/fsmc_sram.h>

#include <stm32f1xx_hal.h>
#include <stm32f1xx_hal_gpio.h>
#include <stm32f1xx_hal_sram.h>
#include <stm32f1xx_hal_gpio_ex.h>

#if !defined(CONFIG_SOC_SERIES_STM32F1X)
#error "zplatform FSMC SRAM requires STM32F1 FSMC"
#endif

static bool sram_ready;

static void fsmc_sram_gpio_init(void)
{
	GPIO_InitTypeDef gpio = {0};

	__HAL_RCC_FSMC_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOE_CLK_ENABLE();
	__HAL_RCC_GPIOF_CLK_ENABLE();
	__HAL_RCC_GPIOG_CLK_ENABLE();
	__HAL_RCC_AFIO_CLK_ENABLE();

	gpio.Mode = GPIO_MODE_AF_PP;
	gpio.Pull = GPIO_NOPULL;
	gpio.Speed = GPIO_SPEED_FREQ_HIGH;

	/* Data: PD0/1/8/9/10/14/15 + PE7-15 */
	gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_8 | GPIO_PIN_9 |
		   GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_15;
	HAL_GPIO_Init(GPIOD, &gpio);

	gpio.Pin = GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 |
		   GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 |
		   GPIO_PIN_15;
	HAL_GPIO_Init(GPIOE, &gpio);

	/* Address: PF0-5,12-15; PG0-5; PD11-13 (A16-A18) */
	gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |
		   GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_12 | GPIO_PIN_13 |
		   GPIO_PIN_14 | GPIO_PIN_15;
	HAL_GPIO_Init(GPIOF, &gpio);

	gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |
		   GPIO_PIN_4 | GPIO_PIN_5;
	HAL_GPIO_Init(GPIOG, &gpio);

	gpio.Pin = GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13;
	HAL_GPIO_Init(GPIOD, &gpio);

	/* NOE / NWE */
	gpio.Pin = GPIO_PIN_4 | GPIO_PIN_5;
	HAL_GPIO_Init(GPIOD, &gpio);

	/* NE3 (SRAM CS) + NE4 (LCD CS, harmless when LCD unused) */
	gpio.Pin = GPIO_PIN_10 | GPIO_PIN_12;
	HAL_GPIO_Init(GPIOG, &gpio);

	/* NBL0 / NBL1 (byte lanes) */
	gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1;
	HAL_GPIO_Init(GPIOE, &gpio);

	/* NADV shares PB7 with I2C1 SDA — disconnect like LCD/Cube. */
	__HAL_AFIO_FSMCNADV_DISCONNECTED();
}

int zplatform_fsmc_sram_init(void)
{
	SRAM_HandleTypeDef hsram = {0};
	FSMC_NORSRAM_TimingTypeDef timing = {0};

	if (sram_ready) {
		return 0;
	}

	fsmc_sram_gpio_init();

	hsram.Instance = FSMC_NORSRAM_DEVICE;
	hsram.Extended = FSMC_NORSRAM_EXTENDED_DEVICE;

	timing.AddressSetupTime = 2;
	timing.AddressHoldTime = 1;
	timing.DataSetupTime = 2;
	timing.BusTurnAroundDuration = 1;
	timing.CLKDivision = 2;
	timing.DataLatency = 2;
	timing.AccessMode = FSMC_ACCESS_MODE_A;

	hsram.Init.NSBank = FSMC_NORSRAM_BANK3;
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

	sram_ready = true;
	return 0;
}

bool zplatform_fsmc_sram_ready(void)
{
	return sram_ready;
}

volatile uint16_t *zplatform_fsmc_sram_base(void)
{
	if (!sram_ready) {
		return NULL;
	}
	return (volatile uint16_t *)ZPLATFORM_FSMC_SRAM_BASE;
}

int zplatform_fsmc_sram_selftest(size_t nbytes)
{
	volatile uint16_t *base = zplatform_fsmc_sram_base();
	size_t nwords;
	size_t i;

	if (base == NULL || nbytes < sizeof(uint16_t)) {
		return -EINVAL;
	}
	if (nbytes > ZPLATFORM_FSMC_SRAM_SIZE) {
		nbytes = ZPLATFORM_FSMC_SRAM_SIZE;
	}
	nwords = nbytes / sizeof(uint16_t);

	for (i = 0; i < nwords; i++) {
		base[i] = (uint16_t)(i ^ 0xA55AU);
	}
	for (i = 0; i < nwords; i++) {
		if (base[i] != (uint16_t)(i ^ 0xA55AU)) {
			return -EIO;
		}
	}
	return 0;
}

#if defined(CONFIG_ZPLATFORM_FSMC_SRAM)
/*
 * LVGL (and any EXT_SRAM section) may touch 0x68000000 during APPLICATION
 * init (priority 90). Bring FSMC bank3 up earlier.
 */
static int zplatform_fsmc_sram_sys_init(void)
{
	return zplatform_fsmc_sram_init();
}

SYS_INIT(zplatform_fsmc_sram_sys_init, POST_KERNEL, 40);
#endif
