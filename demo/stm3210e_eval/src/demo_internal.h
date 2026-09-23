/*
 * Copyright (c) 2026 ZPlatform contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DEMO_INTERNAL_H_
#define DEMO_INTERNAL_H_

#include <stdbool.h>

/** Console smoke: GPIO / buttons / LED. */
void demo_run_gpio(void);

/** Console smoke: buses + tested EVK peripherals. */
void demo_run_peripherals(void);

/** Console smoke: UID / reset / diag. */
void demo_run_diagnostics(void);

#if defined(CONFIG_LVGL)
/** Interactive LVGL UI (splash / IO / peri / threads). Never returns. */
void demo_ui_run(void);
#endif

#endif /* DEMO_INTERNAL_H_ */
