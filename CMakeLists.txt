# Copyright (c) 2026 ZPlatform contributors
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_ZPLATFORM_LIB_COMMON
  lib/common/zplatform_info.c
)
zephyr_library_sources_ifdef(CONFIG_ZPLATFORM_LIB_DIAGNOSTICS
  lib/diagnostics/zplatform_diag.c
)
zephyr_library_sources_ifdef(CONFIG_ZPLATFORM_PROFILING
  lib/diagnostics/zplatform_profile.c
)
zephyr_library_sources_ifdef(CONFIG_ZPLATFORM_SHELL
  lib/common/zplatform_shell.c
)
zephyr_library_sources_ifdef(CONFIG_ZPLATFORM_LED_TRIGGERS
  lib/led/zplatform_led_triggers.c
)
zephyr_library_sources_ifdef(CONFIG_ZPLATFORM_MB895_LCD
  lib/lcd/mb895_lcd.c
  drivers/display/display_mb895.c
)
zephyr_library_sources_ifdef(CONFIG_ZPLATFORM_FSMC_SRAM
  lib/fsmc/fsmc_sram.c
)
zephyr_library_sources_ifdef(CONFIG_ZPLATFORM_SDIO_SD
  lib/sd/sdio_sd.c
)

zephyr_include_directories(include)
