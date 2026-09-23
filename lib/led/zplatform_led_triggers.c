/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Software LED triggers modeled on Linux LED class:
 * heartbeat, cpu, timer, panic.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>
#include <zephyr/fatal.h>
#include <zephyr/sys/util.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <zplatform/led_triggers.h>

enum led_trig {
	LED_TRIG_NONE = 0,
	LED_TRIG_HEARTBEAT,
	LED_TRIG_CPU,
	LED_TRIG_TIMER,
	LED_TRIG_PANIC,
};

struct led_slot {
	struct gpio_dt_spec gpio;
	enum led_trig trig;
	bool ready;
	uint32_t phase_ms;
	bool on;
};

#define TICK_MS 20U
#define TIMER_MS_MIN 40U
#define TIMER_MS_MAX 2000U
#define TIMER_MS_STEP 50U

static struct led_slot slots[4];
static bool started;
static bool panic_latched;
static bool paused;
static uint32_t timer_on_ms = CONFIG_ZPLATFORM_LED_TRIG_TIMER_ON_MS;
static uint32_t timer_off_ms = CONFIG_ZPLATFORM_LED_TRIG_TIMER_OFF_MS;
static struct k_thread trig_thread;
static K_THREAD_STACK_DEFINE(trig_stack, 1024);
static struct k_mutex map_lock;

#if defined(CONFIG_SCHED_THREAD_USAGE) && defined(CONFIG_SCHED_THREAD_USAGE_ALL)
static uint64_t last_exec;
static uint64_t last_total;
static uint8_t cpu_busy_pct;

static void cpu_sample(void)
{
	k_thread_runtime_stats_t stats;
	uint64_t d_exec;
	uint64_t d_total;

	if (k_thread_runtime_stats_cpu_get(0, &stats) != 0) {
		return;
	}

	if (last_exec == 0U) {
		last_exec = stats.execution_cycles;
		last_total = stats.total_cycles;
		return;
	}

	d_exec = stats.execution_cycles - last_exec;
	d_total = stats.total_cycles - last_total;
	last_exec = stats.execution_cycles;
	last_total = stats.total_cycles;

	if (d_exec == 0U) {
		return;
	}

	cpu_busy_pct = (uint8_t)MIN(100U, (d_total * 100U) / d_exec);
}
#else
static uint8_t cpu_busy_pct = 25;

static void cpu_sample(void)
{
}
#endif

static const char *trig_name(enum led_trig t)
{
	switch (t) {
	case LED_TRIG_HEARTBEAT:
		return "HB";
	case LED_TRIG_CPU:
		return "CPU";
	case LED_TRIG_TIMER:
		return "TIM";
	case LED_TRIG_PANIC:
		return "PAN";
	default:
		return "-";
	}
}

static void led_set(struct led_slot *s, bool on)
{
	if (!s->ready) {
		return;
	}
	/* Always drive the pin — avoids soft-state drift if a pattern skipped frames. */
	gpio_pin_set_dt(&s->gpio, on ? 1 : 0);
	s->on = on;
}

static void step_heartbeat(struct led_slot *s)
{
	const uint32_t thump_on = 70;
	const uint32_t thump_gap = 70;
	const uint32_t rest_base = 900;
	uint32_t rest = rest_base - ((uint32_t)cpu_busy_pct * 6U);
	uint32_t cycle;

	if (rest < 200U) {
		rest = 200U;
	}

	cycle = thump_on + thump_gap + thump_on + rest;
	s->phase_ms += TICK_MS;
	if (s->phase_ms >= cycle) {
		s->phase_ms = 0;
	}

	if (s->phase_ms < thump_on) {
		led_set(s, true);
	} else if (s->phase_ms < thump_on + thump_gap) {
		led_set(s, false);
	} else if (s->phase_ms < thump_on + thump_gap + thump_on) {
		led_set(s, true);
	} else {
		led_set(s, false);
	}
}

static void step_timer(struct led_slot *s)
{
	uint32_t gap_ms;
	uint32_t cycle;
	uint32_t phase;
	/* Longer pulse so TIM is obvious vs CPU load flicker on LD2. */
	const uint32_t pulse_ms = 200U;

	k_mutex_lock(&map_lock, K_FOREVER);
	gap_ms = timer_on_ms;
	k_mutex_unlock(&map_lock);

	if (gap_ms < TIMER_MS_MIN) {
		gap_ms = TIMER_MS_MIN;
	}

	cycle = pulse_ms + gap_ms;
	phase = k_uptime_get_32() % cycle;
	led_set(s, phase < pulse_ms);
}

static void step_cpu(struct led_slot *s)
{
	const uint32_t window = 200;
	uint32_t on_ms;

	/*
	 * Only show activity above a floor so idle / button handling does not
	 * look like a "timer" LED that only lights when Joy U/D is pressed.
	 */
	if (cpu_busy_pct < 15U) {
		led_set(s, false);
		return;
	}

	on_ms = ((uint32_t)cpu_busy_pct * window) / 100U;

	s->phase_ms += TICK_MS;
	if (s->phase_ms >= window) {
		s->phase_ms = 0;
	}

	if (on_ms < TICK_MS) {
		on_ms = TICK_MS;
	}

	led_set(s, s->phase_ms < on_ms);
}

static void step_panic(struct led_slot *s)
{
	led_set(s, panic_latched);
}

static void step_slot(struct led_slot *s)
{
	enum led_trig trig;

	k_mutex_lock(&map_lock, K_FOREVER);
	trig = s->trig;
	k_mutex_unlock(&map_lock);

	switch (trig) {
	case LED_TRIG_HEARTBEAT:
		step_heartbeat(s);
		break;
	case LED_TRIG_CPU:
		step_cpu(s);
		break;
	case LED_TRIG_TIMER:
		step_timer(s);
		break;
	case LED_TRIG_PANIC:
		step_panic(s);
		break;
	default:
		break;
	}
}

static void trig_thread_fn(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	while (1) {
		if (!paused && !panic_latched) {
			cpu_sample();
			for (size_t i = 0; i < ARRAY_SIZE(slots); i++) {
				if (slots[i].ready) {
					step_slot(&slots[i]);
				}
			}
		} else if (panic_latched) {
			for (size_t i = 0; i < ARRAY_SIZE(slots); i++) {
				if (slots[i].ready) {
					step_panic(&slots[i]);
				}
			}
		}
		k_msleep(TICK_MS);
	}
}

static bool init_slot(struct led_slot *s, const struct gpio_dt_spec *gpio, enum led_trig trig)
{
	if (gpio->port == NULL) {
		return false;
	}
	if (!gpio_is_ready_dt(gpio)) {
		return false;
	}
	if (gpio_pin_configure_dt(gpio, GPIO_OUTPUT_INACTIVE) != 0) {
		return false;
	}

	s->gpio = *gpio;
	s->trig = trig;
	s->ready = true;
	s->phase_ms = 0;
	s->on = false;
	return true;
}

int zplatform_led_triggers_start(void)
{
	int count = 0;

	if (started) {
		return 0;
	}

	k_mutex_init(&map_lock);

	if (timer_on_ms < TIMER_MS_MIN) {
		timer_on_ms = TIMER_MS_MIN;
	}
	if (timer_off_ms < TIMER_MS_MIN) {
		timer_off_ms = TIMER_MS_MIN;
	}

#if DT_NODE_EXISTS(DT_ALIAS(led0))
	{
		static const struct gpio_dt_spec g =
			GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

		if (init_slot(&slots[0], &g, LED_TRIG_HEARTBEAT)) {
			count++;
		}
	}
#endif
#if DT_NODE_EXISTS(DT_ALIAS(led1))
	{
		static const struct gpio_dt_spec g =
			GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

		if (init_slot(&slots[1], &g, LED_TRIG_CPU)) {
			count++;
		}
	}
#endif
#if DT_NODE_EXISTS(DT_ALIAS(led2))
	{
		static const struct gpio_dt_spec g =
			GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

		if (init_slot(&slots[2], &g, LED_TRIG_TIMER)) {
			count++;
		}
	}
#endif
#if DT_NODE_EXISTS(DT_ALIAS(led3))
	{
		static const struct gpio_dt_spec g =
			GPIO_DT_SPEC_GET(DT_ALIAS(led3), gpios);

		if (init_slot(&slots[3], &g, LED_TRIG_PANIC)) {
			count++;
		}
	}
#endif

	if (count == 0) {
		return -ENODEV;
	}

	k_thread_create(&trig_thread, trig_stack, K_THREAD_STACK_SIZEOF(trig_stack),
			trig_thread_fn, NULL, NULL, NULL, K_PRIO_PREEMPT(10), 0, K_NO_WAIT);
	k_thread_name_set(&trig_thread, "zp_led_trig");
	started = true;
	return 0;
}

void zplatform_led_triggers_on_panic(void)
{
	panic_latched = true;

	for (size_t i = 0; i < ARRAY_SIZE(slots); i++) {
		enum led_trig trig;

		k_mutex_lock(&map_lock, K_FOREVER);
		trig = slots[i].trig;
		k_mutex_unlock(&map_lock);

		if (slots[i].ready && trig == LED_TRIG_PANIC) {
			gpio_pin_set_dt(&slots[i].gpio, 1);
			slots[i].on = true;
		}
	}
}

void zplatform_led_triggers_pause(bool pause)
{
	paused = pause;
}

bool zplatform_led_triggers_paused(void)
{
	return paused;
}

void zplatform_led_triggers_rotate(int dir)
{
	enum led_trig tmp[4];
	int n = 0;

	if (!started || dir == 0) {
		return;
	}

	k_mutex_lock(&map_lock, K_FOREVER);

	for (size_t i = 0; i < ARRAY_SIZE(slots); i++) {
		if (slots[i].ready) {
			tmp[n++] = slots[i].trig;
		}
	}

	if (n < 2) {
		k_mutex_unlock(&map_lock);
		return;
	}

	if (dir > 0) {
		/* right: last → first */
		enum led_trig last = tmp[n - 1];

		for (int i = n - 1; i > 0; i--) {
			tmp[i] = tmp[i - 1];
		}
		tmp[0] = last;
	} else {
		/* left: first → last */
		enum led_trig first = tmp[0];

		for (int i = 0; i < n - 1; i++) {
			tmp[i] = tmp[i + 1];
		}
		tmp[n - 1] = first;
	}

	n = 0;
	for (size_t i = 0; i < ARRAY_SIZE(slots); i++) {
		if (slots[i].ready) {
			slots[i].trig = tmp[n++];
			slots[i].phase_ms = 0;
		}
	}

	k_mutex_unlock(&map_lock);
}

uint32_t zplatform_led_triggers_timer_adjust(int delta_ms)
{
	int32_t next;

	k_mutex_lock(&map_lock, K_FOREVER);
	next = (int32_t)timer_on_ms + delta_ms;
	if (next < (int32_t)TIMER_MS_MIN) {
		next = TIMER_MS_MIN;
	}
	if (next > (int32_t)TIMER_MS_MAX) {
		next = TIMER_MS_MAX;
	}
	timer_on_ms = (uint32_t)next;
	timer_off_ms = (uint32_t)next;

	/* Restart pattern phase on every TIMER-bearing LED. */
	for (size_t i = 0; i < ARRAY_SIZE(slots); i++) {
		if (slots[i].ready && slots[i].trig == LED_TRIG_TIMER) {
			slots[i].phase_ms = 0;
		}
	}
	k_mutex_unlock(&map_lock);

	return (uint32_t)next;
}

uint32_t zplatform_led_triggers_timer_ms(void)
{
	uint32_t v;

	k_mutex_lock(&map_lock, K_FOREVER);
	v = timer_on_ms;
	k_mutex_unlock(&map_lock);
	return v;
}

int zplatform_led_triggers_format_map(char *buf, size_t buflen)
{
	int off = 0;

	if (buf == NULL || buflen == 0) {
		return -EINVAL;
	}

	k_mutex_lock(&map_lock, K_FOREVER);
	for (size_t i = 0; i < ARRAY_SIZE(slots); i++) {
		if (!slots[i].ready) {
			continue;
		}
		off += snprintk(buf + off, buflen > (size_t)off ? buflen - (size_t)off : 0,
				"%s%s", off ? " " : "", trig_name(slots[i].trig));
	}
	k_mutex_unlock(&map_lock);

	if (off <= 0) {
		buf[0] = '\0';
		return -ENODEV;
	}
	return off;
}

#if defined(CONFIG_ZPLATFORM_LED_TRIGGERS)
void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *esf)
{
	ARG_UNUSED(esf);

	zplatform_led_triggers_on_panic();
	k_fatal_halt(reason);
}
#endif
