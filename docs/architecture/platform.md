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
~/zephyrproject/          # west topdir (or $ZPLATFORM_WEST_WORKSPACE)
├── .west/config          # manifest.path = absolute path to zplatform clone
├── zephyr/               # pinned in west.yml
├── bootloader/
└── modules/
```

The ZPlatform git clone can live anywhere (even another drive). No
`zplatform/` junction inside the workspace.

Initialize (default workspace is `~/zephyrproject`):

```bash
./scripts/sh/setup-zephyr.sh --sync
```

Or point elsewhere:

```bash
export ZPLATFORM_WEST_WORKSPACE=/path/to/custom-ws
./scripts/sh/setup-zephyr.sh --sync
```

Manual west (optional):

```bash
# .west/config is written by setup-zephyr; prefer that over west init -l
./scripts/sh/setup-zephyr.sh --sync
```

## Zephyr version

Pinned via `config/toolchain.yml` → `west.yml`: **v4.4.2** (Zephyr 4.4 line).

Use `./scripts/sh/setup-zephyr.sh --check-latest` to discover newer stables without
changing the pin. See [Environment manager](../environment.md).

## DeviceTree vs Kconfig

- **DeviceTree** describes physical hardware (pins, buses, memory).
- **Kconfig** enables software features.
- Application code uses `DEVICE_DT_GET` / aliases — avoid `#ifdef BOARD_...`.

## Adding a board

1. Add `boards/<vendor>/<board>/` following Zephyr board-porting conventions.
2. Prefer reusing upstream SoC DTSI and pinctrl from `hal_stm32`.
3. Keep samples unchanged; rely on aliases (`led0`, `sw0`) and `chosen` nodes.
