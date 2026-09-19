# STM3210E-EVAL

## Hardware

| Item | Value |
|------|-------|
| Board | STMicroelectronics STM3210E-EVAL (MB672) |
| MCU (current) | **STM32F103ZGT6** |
| MCU (older revs) | STM32F103ZET6 (MB672 ≤ D-03) |
| CPU | Arm Cortex-M3 |
| Max CPU frequency | 72 MHz |
| Flash (ZGT6) | 1024 KiB (dual bank, 2 KiB erase) |
| SRAM (ZGT6) | 96 KiB |
| Flash (ZET6) | 512 KiB |
| SRAM (ZET6) | 64 KiB |
| HSE | 8 MHz crystal |

Sources: ST UM0455, product page, ST BSP `stm3210e_eval.h`.

## Upstream Zephyr support

**Upstream Zephyr does not provide `stm3210e_eval`.**

Nearby upstream boards:

| Board | SoC | Notes |
|-------|-----|-------|
| `stm3210c_eval` | STM32F107VC | Different MCU (connectivity line + Ethernet) |
| `nucleo_f103rb` | STM32F103RB | Same series, different package/board |
| `waveshare/open103z` | STM32F103ZE | Similar Z package LED pins |

ZPlatform therefore ships **out-of-tree** board support under
`boards/st/stm3210e_eval/`, reusing:

- `st/f1/stm32f103Xg.dtsi` (1 MiB / 96 KiB)
- `st/f1/stm32f103z(f-g)tx-pinctrl.dtsi`
- `SOC_STM32F103XE` Kconfig
- OpenOCD / CubeProgrammer / J-Link runners via Zephyr board.cmake helpers

## Peripherals described in DeviceTree

| Function | Wiring | Notes |
|----------|--------|-------|
| Console UART | USART1 PA9/PA10 | RS-232 on CN12 |
| LEDs LD1–LD4 | PF6–PF9 | Active high |
| User Key | PG8 | `sw0` |
| Tamper | PC13 | |
| Wakeup | PA0 | Shares EXTI0 |
| I2C1 | PB6/PB7 | STLM75 @ 0x48 |
| SPI1 | PA5/PA6/PA7, CS PB2 | SPI NOR footprint |
| USB FS | PA11/PA12 | Device |
| ADC1 | PA1 (IN1) | |
| PWM | TIM1_CH1 PA8 | |
| RTC | LSI | |
| IWDG | on-chip | |
| Ethernet | — | **Not present** on STM32F103 |

FSMC memories (NOR/NAND/SRAM), LCD, audio codec, CAN, SDIO, and joystick
are present on the PCB but not all are enabled in the initial DT. Add them
incrementally with verified pinmux.

## Debug / ST-Link

STM3210E-EVAL exposes **JTAG/SWD** on the debug connector. Unlike many Nucleo
boards, use an **external ST-Link/V2** (or J-Link).

Default Zephyr runner: OpenOCD (`support/openocd.cfg`).

```bash
./scripts/build.sh demo --board stm3210e_eval
./scripts/flash.sh
./scripts/debug.sh
```

## Build

```bash
./scripts/build.sh sample board_bringup --board stm3210e_eval
./scripts/build.sh demo --board stm3210e_eval
```

Artifacts: `build/stm3210e_eval/<app>/<cfg>/zephyr/zephyr.{elf,bin,hex,map}`

## MCUboot memory layout (ZGT6, 1024 KiB)

Erase size: **2 KiB**.

| Region | Offset | Size | Purpose |
|--------|--------|------|---------|
| MCUboot | `0x00000` | 64 KiB | Bootloader |
| Slot 0 | `0x10000` | 464 KiB | Primary image |
| Slot 1 | `0x84000` | 464 KiB | Update image |
| Storage | `0xF8000` | 32 KiB | Settings / NVS |

Absolute Flash base: `0x08000000`.

Swap mode: `CONFIG_BOOT_SWAP_USING_MOVE=y` (no scratch partition).

See `support/mcuboot.conf`, `stm3210e_eval_mcuboot.overlay`, and
`scripts/mcuboot_demo.sh`.

## Profiling

```bash
./scripts/profile.sh demo --board stm3210e_eval
```

Uses DWT CYCCNT (Cortex-M3), thread runtime stats, and stack watermarks.
SystemView is optional and not enabled by default.

## Ztest

```bash
./scripts/test.sh
./scripts/test.sh platform/smoke
```

Default script mode is **build-only** for `stm3210e_eval` (no claim of
hardware pass). Use Twister device testing when hardware is attached.

## Known limitations

- Older ZET6 boards need a DT/memory variant (not shipped yet).
- External memories / LCD / audio / CAN / SDIO not enabled initially.
- Ethernet unavailable on this MCU.
- Hardware bring-up requires physical ST-Link + board.

## Hardware validation

```text
Hardware validation: NOT PERFORMED
```

Report physical results separately after lab testing.
