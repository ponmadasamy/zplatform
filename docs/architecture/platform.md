# ZPlatform Architecture

ZPlatform is a Zephyr **module** (and west manifest project) that provides:

- out-of-tree board support
- reusable platform libraries
- shared, DeviceTree-gated peripheral samples
- **one full demo application per board**
- developer scripts for build / flash / debug / profile / test
- MCUboot integration for selected boards

## Layering

```text
                     Application
                          │
              ┌───────────┴───────────┐
              │                       │
        Demo/<board>              Samples
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

### Board vs sample vs demo

| Layer | Owns | Does not own |
|-------|------|----------------|
| Board (`boards/<vendor>/<board>/`) | Pins, clocks, partitions, aliases, minimal bring-up Kconfig | App feature lists (SHELL, sensors-for-demo, …) |
| Sample (`samples/<name>/`) | One peripheral story; portable source | Hard-coded pins / `#ifdef BOARD_…` |
| Demo (`demo/<board>/`) | Full showcase for that hardware | Other boards’ peripherals |

```text
board.dts + board_defconfig
        │
        ├─► samples/<name>/prj.conf  (+ optional boards/<board>.conf|.overlay)
        │         source: DT_ALIAS / DT_NODE_EXISTS
        │
        └─► demo/<board>/prj.conf    (full feature set for that EVK)
```

**Rule:** DeviceTree describes presence; Kconfig enables software. Application
code uses `DT_ALIAS` / `DT_NODE_EXISTS` / `DEVICE_DT_GET_OR_NULL` — never
`#ifdef BOARD_…` in business logic.

### Conf / DTS merge order

**Samples**

1. Board `*_defconfig`
2. Sample `prj.conf`
3. Optional `samples/<name>/boards/<board>.conf` (Zephyr auto-merge)
4. Optional `samples/<name>/boards/<board>.overlay` (rare: enable a disabled node)

**Demos**

1. Board `*_defconfig`
2. `demo/<board>/prj.conf`
3. `demo/<board>/prj_mcuboot.conf` (or script `-DCONFIG_*`) when `-Mcuboot`
4. Board-package overlays for policy (e.g. MCUboot code partition)

Board `*_defconfig` stays **minimal** (clock, GPIO, console UART, pinctrl,
hwinfo). Demo/sample features live in the app `prj.conf`.

### Build selection

```text
./scripts/ps1/build.ps1 demo -Board stm3210e_eval
  → sources demo/stm3210e_eval/
  → build/stm3210e_eval/demo/default/

./scripts/ps1/build.ps1 sample gpio -Board stm3210e_eval
  → sources samples/gpio/
  → build/stm3210e_eval/sample/gpio/default/

./scripts/ps1/build.ps1 demo -Board stm3210e_eval -Mcuboot
  → build/stm3210e_eval/demo/mcuboot/
```

Each board / kind / config combination has its **own** build directory so
switching flash/debug targets never forces a rebuild.

```text
build/<board>/
  demo/<config>/                 # default | debug | release | mcuboot | profile
  sample/<name>/<config>/
  boot/mcuboot/<config>/         # bootloader (usually default)
.zplatform-last-build            # pointer only — flash/debug read this
```

Flash and debug **only select** an existing tree:

```text
.\scripts\ps1\flash.ps1 demo -Config default
.\scripts\ps1\flash.ps1 demo -Config mcuboot
.\scripts\ps1\debug.ps1 sample gpio -Config default
```

Missing `demo/<board>/` is a hard error. Missing build output is a hard error
listing available configs — scripts never silently pick another config.

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

## Zephyr version

Pinned via `config/toolchain.yml` → `west.yml`: **v4.4.2** (Zephyr 4.4 line).

Use `./scripts/sh/setup-zephyr.sh --check-latest` to discover newer stables without
changing the pin. See [Environment manager](../environment.md).

## Adding a board

1. Add `boards/<vendor>/<board>/` following Zephyr board-porting conventions.
2. Prefer reusing upstream SoC DTSI and pinctrl from `hal_stm32`.
3. Define aliases (`led0`, `sw0`, …) and `chosen` nodes on the board.
4. Keep shared samples unchanged; they gate on DT aliases.
5. Add `demo/<board>/` with a `prj.conf` that enables everything that board can show.
