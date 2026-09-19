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

zephyr_include_directories(include)
