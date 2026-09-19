# ZPlatform Architecture

ZPlatform is a Zephyr **module** (and west manifest project) that provides:

- out-of-tree board support
- reusable platform libraries
- board-agnostic samples
- a full platform demo
- developer scripts for build / flash / debug / profile / test
- MCUboot integration for selected boards

## Layering

```text
                     Application
                          │
              ┌───────────┴───────────┐
              │                       │
             Demo                   Samples
              │                       │
              └───────────┬───────────┘
                          │
                   ZPlatform APIs
                          │
             ┌────────────┴────────────┐
             │                         │
       Platform Services          Drivers
             │                         │
             └────────────┬────────────┘
                          │
                    Zephyr APIs
                          │
                    Zephyr HAL
                          │
                    STM32 Hardware
```

## Ownership

| Area | Owner |
|------|-------|
| Kernel, upstream drivers, HAL, networking, FS, USB, BT | Zephyr |
| Out-of-tree boards, drivers, platform services, demos, scripts | ZPlatform |

Do not fork Zephyr. Do not vendor the Zephyr tree inside this repository.

## West workspace

Preferred layout:

```text
workspace/
├── .west/
├── zephyr/          # upstream (revision pinned in west.yml)
├── bootloader/
├── modules/
└── zplatform/       # this repository
```

Initialize:

```bash
mkdir zplatform-ws && cd zplatform-ws
west init -m git@github.com:ponmadasamy/zplatform.git
west update
```

Or with an existing clone:

```bash
mkdir zplatform-ws && cd zplatform-ws
west init -l /path/to/zplatform
west update
```

## Zephyr version

Pinned in `west.yml`: **v4.4.0** (latest stable at workspace creation).

## DeviceTree vs Kconfig

- **DeviceTree** describes physical hardware (pins, buses, memory).
- **Kconfig** enables software features.
- Application code uses `DEVICE_DT_GET` / aliases — avoid `#ifdef BOARD_...`.

## Adding a board

1. Add `boards/<vendor>/<board>/` following Zephyr board-porting conventions.
2. Prefer reusing upstream SoC DTSI and pinctrl from `hal_stm32`.
3. Keep samples unchanged; rely on aliases (`led0`, `sw0`) and `chosen` nodes.
