/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/devicetree.h>
#include <zephyr/linker/linker-defs.h>
#include <string.h>

#include <zplatform/diag.h>

static void thread_count_cb(const struct k_thread *thread, void *user_data)
{
	ARG_UNUSED(thread);
	(*(uint32_t *)user_data)++;
}

int zplatform_get_mem_info(struct zplatform_mem_info *info)
{
	if (info == NULL) {
		return -EINVAL;
	}

	memset(info, 0, sizeof(*info));

#if DT_NODE_EXISTS(DT_CHOSEN(zephyr_sram))
	info->sram_total = DT_REG_SIZE(DT_CHOSEN(zephyr_sram));
#elif DT_NODE_EXISTS(DT_NODELABEL(sram0))
	info->sram_total = DT_REG_SIZE(DT_NODELABEL(sram0));
#endif

#if DT_NODE_EXISTS(DT_CHOSEN(zephyr_flash))
	info->flash_total = DT_REG_SIZE(DT_CHOSEN(zephyr_flash));
#elif DT_NODE_EXISTS(DT_NODELABEL(flash0))
	info->flash_total = DT_REG_SIZE(DT_NODELABEL(flash0));
#endif

	info->flash_used = (size_t)__rom_region_size;
	info->sram_used = (size_t)((uintptr_t)__kernel_ram_end -
				   (uintptr_t)__kernel_ram_start);

	if (info->sram_total >= info->sram_used) {
		info->sram_free = info->sram_total - info->sram_used;
	}

	return 0;
}

int zplatform_get_thread_stats(struct zplatform_thread_stats *stats)
{
	uint32_t count = 0;

	if (stats == NULL) {
		return -EINVAL;
	}

	memset(stats, 0, sizeof(*stats));
	k_thread_foreach(thread_count_cb, &count);
	stats->thread_count = count;

	return 0;
}

void zplatform_diag_report(void)
{
	struct zplatform_mem_info mem;
	struct zplatform_thread_stats thr;

	if (zplatform_get_mem_info(&mem) == 0) {
		printk("Diagnostics\n");
		printk(" Flash: %u / %u bytes used\n",
		       (unsigned int)mem.flash_used,
		       (unsigned int)mem.flash_total);
		printk(" RAM:   %u / %u bytes used (%u free)\n",
		       (unsigned int)mem.sram_used,
		       (unsigned int)mem.sram_total,
		       (unsigned int)mem.sram_free);
	}

	if (zplatform_get_thread_stats(&thr) == 0) {
		printk(" Threads: %u\n", thr.thread_count);
	}
}
