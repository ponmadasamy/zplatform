# SPDX-License-Identifier: Apache-2.0

# STM3210E-EVAL uses an external ST-Link/V2 (or compatible) on the
# JTAG/SWD connector. Prefer OpenOCD; also support CubeProgrammer and J-Link.

board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")
board_runner_args(jlink "--device=STM32F103ZG" "--speed=4000")

include(${ZEPHYR_BASE}/boards/common/openocd-stm32.board.cmake)
include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
