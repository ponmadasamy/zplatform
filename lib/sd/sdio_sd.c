/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * STM3210E-EVAL microSD (CN13) over SDIO — Cube BSP polling path.
 */

#include <zephyr/kernel.h>
#include <errno.h>
#include <string.h>

#include <zplatform/sdio_sd.h>

#include <stm32f1xx_hal.h>
#include <stm32f1xx_hal_gpio.h>
#include <stm32f1xx_hal_sd.h>

#if !defined(CONFIG_SOC_SERIES_STM32F1X)
#error "zplatform SDIO SD requires STM32F1 SDIO"
#endif

#define SD_DETECT_PIN  GPIO_PIN_11
#define SD_DETECT_PORT GPIOF

static SD_HandleTypeDef hsd;
static bool sd_ready;
static bool detect_gpio_ready;

static void sd_detect_gpio_init(void)
{
	GPIO_InitTypeDef gpio = {0};

	if (detect_gpio_ready) {
		return;
	}

	__HAL_RCC_GPIOF_CLK_ENABLE();

	gpio.Mode = GPIO_MODE_INPUT;
	gpio.Pull = GPIO_PULLUP;
	gpio.Speed = GPIO_SPEED_FREQ_HIGH;
	gpio.Pin = SD_DETECT_PIN;
	HAL_GPIO_Init(SD_DETECT_PORT, &gpio);

	detect_gpio_ready = true;
}

static void sd_msp_init(void)
{
	GPIO_InitTypeDef gpio = {0};

	__HAL_RCC_SDIO_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();

	gpio.Mode = GPIO_MODE_AF_PP;
	gpio.Pull = GPIO_PULLUP;
	gpio.Speed = GPIO_SPEED_FREQ_HIGH;

	/* PC8-12: D0 D1 D2 D3 CLK */
	gpio.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 |
		   GPIO_PIN_12;
	HAL_GPIO_Init(GPIOC, &gpio);

	/* PD2: CMD */
	gpio.Pin = GPIO_PIN_2;
	HAL_GPIO_Init(GPIOD, &gpio);

	sd_detect_gpio_init();
}

bool zplatform_sdio_sd_detected(void)
{
	sd_detect_gpio_init();
	/* Active low: present when pin is reset. */
	return HAL_GPIO_ReadPin(SD_DETECT_PORT, SD_DETECT_PIN) == GPIO_PIN_RESET;
}

int zplatform_sdio_sd_init(void)
{
	if (sd_ready) {
		return 0;
	}

	if (!zplatform_sdio_sd_detected()) {
		return -ENODEV;
	}

	memset(&hsd, 0, sizeof(hsd));
	hsd.Instance = SDIO;
	hsd.Init.ClockEdge = SDIO_CLOCK_EDGE_RISING;
	hsd.Init.ClockBypass = SDIO_CLOCK_BYPASS_DISABLE;
	hsd.Init.ClockPowerSave = SDIO_CLOCK_POWER_SAVE_DISABLE;
	hsd.Init.BusWide = SDIO_BUS_WIDE_1B;
	hsd.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
	hsd.Init.ClockDiv = SDIO_TRANSFER_CLK_DIV;

	sd_msp_init();

	if (HAL_SD_Init(&hsd) != HAL_OK) {
		return -EIO;
	}

	/*
	 * Stay on 1-bit for bring-up: 4-bit needs JP22 open (PC11=D3).
	 * Wide-bus ACMD6 can succeed even when D3 is muxed to IrDA, then
	 * block reads fail — so do not enable 4-bit until Jumpers confirmed.
	 */
	sd_ready = true;
	return 0;
}

bool zplatform_sdio_sd_ready(void)
{
	return sd_ready;
}

int zplatform_sdio_sd_get_info(struct zplatform_sd_info *info)
{
	HAL_SD_CardInfoTypeDef ci;

	if (!sd_ready || info == NULL) {
		return -EINVAL;
	}
	if (HAL_SD_GetCardInfo(&hsd, &ci) != HAL_OK) {
		return -EIO;
	}

	info->block_nbr = ci.BlockNbr;
	info->block_size = ci.BlockSize;
	info->log_block_nbr = ci.LogBlockNbr;
	info->log_block_size = ci.LogBlockSize;
	info->card_type = ci.CardType;
	info->card_version = ci.CardVersion;
	info->class = ci.Class;
	info->rel_card_add = ci.RelCardAdd;
	return 0;
}

int zplatform_sdio_sd_read(uint32_t block, void *buf, uint32_t count)
{
	if (!sd_ready || buf == NULL || count == 0U) {
		return -EINVAL;
	}
	if (HAL_SD_ReadBlocks(&hsd, buf, block, count, 10000U) != HAL_OK) {
		return -EIO;
	}
	while (HAL_SD_GetCardState(&hsd) != HAL_SD_CARD_TRANSFER) {
		/* wait */
	}
	return 0;
}

int zplatform_sdio_sd_write(uint32_t block, const void *buf, uint32_t count)
{
	if (!sd_ready || buf == NULL || count == 0U) {
		return -EINVAL;
	}
	if (HAL_SD_WriteBlocks(&hsd, (uint8_t *)buf, block, count, 10000U) !=
	    HAL_OK) {
		return -EIO;
	}
	while (HAL_SD_GetCardState(&hsd) != HAL_SD_CARD_TRANSFER) {
		/* wait */
	}
	return 0;
}
