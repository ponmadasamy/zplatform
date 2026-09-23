/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Kernel threads — live stack + CPU usage with background load workers.
 */

#include <stdio.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <lvgl.h>

#include "ui_theme.h"
#include "ui_screens.h"

#define UI_THREAD_MAX 6
#define LOAD_STACK_SIZE 1024

struct thr_row {
	lv_obj_t *name;
	lv_obj_t *bar;
	lv_obj_t *pct;
	char label[16];
};

static lv_obj_t *hdr;
static struct thr_row rows[UI_THREAD_MAX];
static uint32_t last_ms;
static int row_count;
static uint32_t load_ticks;

struct thr_scan {
	char name[12];
	int used_pct;
	int cpu_pct;
};

static struct thr_scan scan_buf[UI_THREAD_MAX];
static int scan_n;

static K_THREAD_STACK_DEFINE(load_lo_stack, LOAD_STACK_SIZE);
static K_THREAD_STACK_DEFINE(load_hi_stack, LOAD_STACK_SIZE);
static struct k_thread load_lo_data;
static struct k_thread load_hi_data;
static bool workers_started;

static void load_lo_entry(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	while (1) {
		volatile uint32_t x = 0;

		for (uint32_t i = 0; i < 8000U; i++) {
			x += i;
		}
		load_ticks++;
		k_msleep(40);
	}
}

static void load_hi_entry(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	while (1) {
		volatile uint32_t x = 1;

		for (uint32_t i = 0; i < 40000U; i++) {
			x = (x * 1664525U) + 1013904223U;
		}
		load_ticks += 3U;
		k_msleep(15);
	}
}

static void ensure_workers(void)
{
	if (workers_started) {
		return;
	}
	workers_started = true;

	k_thread_create(&load_lo_data, load_lo_stack,
			K_THREAD_STACK_SIZEOF(load_lo_stack), load_lo_entry,
			NULL, NULL, NULL, 8, 0, K_NO_WAIT);
	k_thread_name_set(&load_lo_data, "load_lo");

	k_thread_create(&load_hi_data, load_hi_stack,
			K_THREAD_STACK_SIZEOF(load_hi_stack), load_hi_entry,
			NULL, NULL, NULL, 7, 0, K_NO_WAIT);
	k_thread_name_set(&load_hi_data, "load_hi");
}

static void thr_cb(const struct k_thread *thread, void *user)
{
	ARG_UNUSED(user);

	if (scan_n >= UI_THREAD_MAX) {
		return;
	}

	size_t unused = 0;
	size_t size = thread->stack_info.size;
	int used = 0;
	int cpu = 0;

#if defined(CONFIG_INIT_STACKS) && defined(CONFIG_THREAD_STACK_INFO)
	(void)k_thread_stack_space_get(thread, &unused);
#endif
	if (size > 0U) {
		size_t used_b = size > unused ? size - unused : 0U;

		used = (int)((used_b * 100U) / size);
	}

#if defined(CONFIG_SCHED_THREAD_USAGE)
	{
		k_thread_runtime_stats_t stats;
		k_thread_runtime_stats_t all;
		int rc = k_thread_runtime_stats_get((k_tid_t)thread, &stats);
		int rca = k_thread_runtime_stats_all_get(&all);

		if (rc == 0 && rca == 0 && all.execution_cycles > 0U) {
			cpu = (int)((stats.execution_cycles * 100ULL) /
				    all.execution_cycles);
			if (cpu > 100) {
				cpu = 100;
			}
		}
	}
#endif

	const char *n = thread->name[0] ? thread->name : "?";

	strncpy(scan_buf[scan_n].name, n, sizeof(scan_buf[scan_n].name) - 1U);
	scan_buf[scan_n].name[sizeof(scan_buf[scan_n].name) - 1U] = '\0';
	scan_buf[scan_n].used_pct = used;
	scan_buf[scan_n].cpu_pct = cpu;
	scan_n++;
}

void ui_threads_create(lv_obj_t *parent)
{
	lv_obj_t *title;

	ensure_workers();
	ui_style_screen(parent);

	title = lv_label_create(parent);
	lv_label_set_text(title, "Threads (live)");
	lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
	lv_obj_set_style_text_color(title, ui_col_cyan(), 0);
	lv_obj_align(title, LV_ALIGN_TOP_LEFT, 8, 4);

	hdr = lv_label_create(parent);
	lv_label_set_text(hdr, "…");
	lv_obj_set_style_text_font(hdr, &lv_font_montserrat_12, 0);
	lv_obj_set_style_text_color(hdr, ui_col_amber(), 0);
	lv_obj_align(hdr, LV_ALIGN_TOP_LEFT, 8, 24);

	for (int i = 0; i < UI_THREAD_MAX; i++) {
		int y = 44 + i * 30;

		rows[i].name = lv_label_create(parent);
		lv_label_set_text(rows[i].name, "");
		lv_obj_set_style_text_font(rows[i].name, &lv_font_montserrat_12,
					   0);
		lv_obj_set_style_text_color(rows[i].name, ui_col_snow(), 0);
		lv_obj_align(rows[i].name, LV_ALIGN_TOP_LEFT, 8, y);

		rows[i].bar = lv_bar_create(parent);
		lv_obj_set_size(rows[i].bar, 140, 10);
		lv_obj_align(rows[i].bar, LV_ALIGN_TOP_LEFT, 110, y + 2);
		lv_bar_set_range(rows[i].bar, 0, 100);
		lv_obj_set_style_bg_color(rows[i].bar, lv_color_hex(0x1A2740),
					  LV_PART_MAIN);
		lv_obj_set_style_bg_color(rows[i].bar, ui_col_amber(),
					  LV_PART_INDICATOR);

		rows[i].pct = lv_label_create(parent);
		lv_label_set_text(rows[i].pct, "");
		lv_obj_set_style_text_font(rows[i].pct, &lv_font_montserrat_12,
					   0);
		lv_obj_set_style_text_color(rows[i].pct, ui_col_azure(), 0);
		lv_obj_align(rows[i].pct, LV_ALIGN_TOP_RIGHT, -8, y);
	}

	lv_obj_t *hint = lv_label_create(parent);
	lv_label_set_text(hint, "Bars=CPU%  |  Joy L/R screens");
	lv_obj_set_style_text_font(hint, &lv_font_montserrat_12, 0);
	lv_obj_set_style_text_color(hint, ui_col_azure(), 0);
	lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -4);

	last_ms = 0;
	row_count = 0;
}

void ui_threads_tick(uint32_t now_ms)
{
	char buf[36];
	uint32_t up_s;

	if (last_ms != 0U && (now_ms - last_ms) < 250U) {
		return;
	}
	last_ms = now_ms;

	scan_n = 0;
	k_thread_foreach(thr_cb, NULL);
	row_count = scan_n;

	up_s = now_ms / 1000U;
	snprintf(buf, sizeof(buf), "up %02u:%02u:%02u  load %u",
		 (up_s / 3600U) % 100U, (up_s / 60U) % 60U, up_s % 60U,
		 load_ticks);
	lv_label_set_text(hdr, buf);

	for (int i = 0; i < UI_THREAD_MAX; i++) {
		if (i < row_count) {
			int bar = scan_buf[i].cpu_pct;

			if (bar == 0) {
				bar = scan_buf[i].used_pct;
			}
			snprintf(rows[i].label, sizeof(rows[i].label), "%s",
				 scan_buf[i].name);
			lv_label_set_text(rows[i].name, rows[i].label);
			lv_bar_set_value(rows[i].bar, bar, LV_ANIM_OFF);
			snprintf(buf, sizeof(buf), "%d/%d",
				 scan_buf[i].cpu_pct, scan_buf[i].used_pct);
			lv_label_set_text(rows[i].pct, buf);
			lv_obj_clear_flag(rows[i].name, LV_OBJ_FLAG_HIDDEN);
			lv_obj_clear_flag(rows[i].bar, LV_OBJ_FLAG_HIDDEN);
			lv_obj_clear_flag(rows[i].pct, LV_OBJ_FLAG_HIDDEN);
		} else {
			lv_obj_add_flag(rows[i].name, LV_OBJ_FLAG_HIDDEN);
			lv_obj_add_flag(rows[i].bar, LV_OBJ_FLAG_HIDDEN);
			lv_obj_add_flag(rows[i].pct, LV_OBJ_FLAG_HIDDEN);
		}
	}
}
