# SPDX-License-Identifier: Apache-2.0

# STM3210E-EVAL uses an external ST-Link (V3MINI etc.) on CN9.
# Prefer STM32CubeProgrammer for flash (reliable with STLINK-V3MINI);
# keep OpenOCD as debugger / fallback flasher (SWD by default).

board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")
board_runner_args(openocd "--config=${CMAKE_CURRENT_LIST_DIR}/support/openocd-swd.cfg")
board_runner_args(jlink "--device=STM32F103ZG" "--speed=4000")

include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd-stm32.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
