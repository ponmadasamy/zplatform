/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <cmsis_core.h>

#include <zplatform/profile.h>

void zplatform_profile_init(void)
{
#if defined(CONFIG_CPU_CORTEX_M) && defined(DWT)
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	DWT->CYCCNT = 0;
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
#endif
}

uint32_t zplatform_profile_cycles(void)
{
#if defined(CONFIG_CPU_CORTEX_M) && defined(DWT)
	return DWT->CYCCNT;
#else
	return k_cycle_get_32();
#endif
}

uint32_t zplatform_profile_elapsed(uint32_t start, uint32_t end)
{
	return end - start;
}
