# Scripts

Host entry points for environment setup, build, clean, flash, debug, profile,
test, and high-level demos. Shared logic lives in `env/zplatform_env.py` and
`*/common.*`.

```text
scripts/
├── sh/     # Linux / macOS / Git Bash
├── ps1/    # Windows PowerShell
├── cmd/    # Windows cmd.exe (when PowerShell is blocked)
└── env/    # Shared Python environment manager
```

Pick the folder for your host — no wrappers. Every script supports `-Help` /
`--help`.

## Shell parity

| Script | ps1 | sh | cmd |
|--------|:---:|:--:|:---:|
| `setup-zephyr` | ✓ | ✓ | ✓ |
| `build` | ✓ | ✓ | ✓ |
| `clean` | ✓ | ✓ | ✓ |
| `flash` | ✓ | ✓ | ✓ |
| `debug` | ✓ | ✓ | ✓ |
| `profile` | ✓ | ✓ | ✓ |
| `test` | ✓ | ✓ | ✓ |
| `demo` | ✓ | ✓ | — |
| `mcuboot_demo` | ✓ full | build / clean only | — |

- **cmd** has no `demo` / `mcuboot_demo` — use `build` + `flash` (and for MCUboot
  boot+app programming, prefer `mcuboot_demo.ps1 -Flash`).
- **sh `mcuboot_demo`** builds bootloader + signed v1/v2; flash/upgrade is
  PowerShell-only (`mcuboot_demo.ps1 -Flash [-Upgrade]`).

### Flag naming (ps1 ↔ sh/cmd)

| Meaning | ps1 | sh / cmd |
|---------|-----|----------|
| Board | `-Board` | `--board` |
| Before-build wipe this config | `-Clean` | `--clean` |
| Wipe entire `build/<board>/` then rebuild | `-CleanAll` (demo / mcuboot_demo only) | `--clean-all` |
| Debug **Kconfig** on `build` | `-DebugBuild` | `--debug` |
| Debug **Kconfig** on `demo` | `-DebugBuild` | `--debug-cfg` |
| Debug **session** on `demo` | `-DebugSession` | `--debug` |
| Release Kconfig | `-Release` | `--release` |
| MCUboot slot0 + sign (`build`) | `-Mcuboot` | `--mcuboot` |
| Flash config / last-build | `-Config` | `--config` |
| Full chip erase | `-Erase` | `--erase` |
| Runner override | `-Runner` | `--runner` |
| Twister native | `-Native` | `--native` |

## Build outputs

Isolated trees (flash/debug never rebuild — they only select an existing tree):

| Kind | Path |
|------|------|
| demo | `build/<board>/demo/<config>/` |
| sample | `build/<board>/sample/<name>/<config>/` |
| boot (MCUboot) | `build/<board>/boot/mcuboot/default/` (or under west workspace if cross-drive) |

Configs: `default` | `debug` | `release` | `mcuboot` | `profile`.

Empty `flash` / `debug` (no app args) uses `build/.zplatform-last-build`.

## setup-zephyr

```text
--status / -Status          Report environment
--ensure / -Ensure          Create/update .venv + deps (default with status)
--sync / -Sync              west update to locked Zephyr revision
--sync-manifest             Rewrite west.yml revision from toolchain.yml
--check-latest / -CheckLatest / -Update
```

```powershell
.\scripts\ps1\setup-zephyr.ps1 -Status
.\scripts\ps1\setup-zephyr.ps1 -Sync
```

```bash
./scripts/sh/setup-zephyr.sh --status
./scripts/sh/setup-zephyr.sh --sync
```

```bat
scripts\cmd\setup-zephyr.cmd --status
```

Default west workspace: `~/zephyrproject` (override `ZPLATFORM_WEST_WORKSPACE`).
Existing SDK installs are never uninstalled.

## build

Kinds: `demo` | `sample <name>`. The current sample directories are
`board_bringup`, `gpio`, `sensor`, `spi_flash`, `adc`, `pwm`, `lcd`,
`ext_sram`, `microsd`, `lvgl`, `rtc`, `dac`, `can`, and `usb_cdc`.
The build scripts resolve sample names from `samples/<name>/`.

| Flag | Config | Behavior |
|------|--------|----------|
| *(none)* | `default` | Whole-flash `code_partition` @ `0x08000000` |
| `-DebugBuild` / `--debug` | `debug` | Same map; debug Kconfig |
| `-Release` / `--release` | `release` | Same map; size opts |
| `-Mcuboot` / `--mcuboot` | `mcuboot` | Slot0 link + west/imgtool → `zephyr.signed.bin` |
| `profile` script | `profile` | Same map; profiling Kconfig |

```powershell
.\scripts\ps1\build.ps1 demo -Board stm3210e_eval
.\scripts\ps1\build.ps1 demo -Board stm3210e_eval -DebugBuild
.\scripts\ps1\build.ps1 demo -Board stm3210e_eval -Release
.\scripts\ps1\build.ps1 demo -Board stm3210e_eval -Mcuboot
.\scripts\ps1\build.ps1 demo -Board stm3210e_eval -Clean
.\scripts\ps1\build.ps1 sample gpio -Board stm3210e_eval
.\scripts\ps1\profile.ps1 demo -Board stm3210e_eval
```

```bash
./scripts/sh/build.sh demo --board stm3210e_eval --mcuboot
./scripts/sh/build.sh sample gpio --board stm3210e_eval
./scripts/sh/profile.sh demo --board stm3210e_eval
```

```bat
scripts\cmd\build.cmd demo --board stm3210e_eval --mcuboot
scripts\cmd\build.cmd sample gpio --board stm3210e_eval
scripts\cmd\profile.cmd demo --board stm3210e_eval
```

## demo (ps1 / sh)

Standalone per-board app (`demo/<board>/`), whole-flash link — not MCUboot.
Default action: build. Flash auto-builds if the config tree is missing.

```powershell
.\scripts\ps1\demo.ps1 -Board stm3210e_eval
.\scripts\ps1\demo.ps1 -Board stm3210e_eval -Clean
.\scripts\ps1\demo.ps1 -Board stm3210e_eval -CleanAll
.\scripts\ps1\demo.ps1 -Build -Flash
.\scripts\ps1\demo.ps1 -Flash
.\scripts\ps1\demo.ps1 -DebugSession
.\scripts\ps1\demo.ps1 -DebugBuild          # config=debug
.\scripts\ps1\demo.ps1 -Release
```

```bash
./scripts/sh/demo.sh --board stm3210e_eval
./scripts/sh/demo.sh --clean --build --flash
./scripts/sh/demo.sh --clean-all
./scripts/sh/demo.sh --debug-cfg
./scripts/sh/demo.sh --debug                # debug session
```

## mcuboot_demo

Bootloader + signed slot0 app (v1) + signed v2 for upgrade demos.

```powershell
.\scripts\ps1\mcuboot_demo.ps1 -Board stm3210e_eval -Build
.\scripts\ps1\mcuboot_demo.ps1 -Clean -Build
.\scripts\ps1\mcuboot_demo.ps1 -CleanAll -Build
.\scripts\ps1\mcuboot_demo.ps1 -Build -Flash
.\scripts\ps1\mcuboot_demo.ps1 -Build -Flash -Upgrade
.\scripts\ps1\mcuboot_demo.ps1 -Flash -Upgrade    # reuse images
```

```bash
./scripts/sh/mcuboot_demo.sh --board stm3210e_eval --build
./scripts/sh/mcuboot_demo.sh --clean --build
# Hardware: ./scripts/ps1/mcuboot_demo.ps1 -Flash [-Upgrade]
```

App-only signed rebuild (bootloader already on device):

```powershell
.\scripts\ps1\build.ps1 demo -Board stm3210e_eval -Mcuboot
.\scripts\ps1\flash.ps1 demo -Config mcuboot
```

Keys: [`keys/README.md`](../keys/README.md). Board map:
[`docs/boards/stm3210e_eval.md`](../docs/boards/stm3210e_eval.md).

## clean

| Scope | Invocation | Effect |
|-------|------------|--------|
| Targeted | `clean demo [-Config CFG]` / `sample <name>` / `boot` | One config, or all configs for that app if config omitted |
| Board | `clean -Board BOARD` / `--board` | Entire `build/<board>/` |
| All | `clean -All` / `--all` | Everything under `build/` |

Before-build (not the standalone clean script):

| Flag | Where | Effect |
|------|-------|--------|
| `-Clean` / `--clean` | `build`, `demo`, `mcuboot_demo` | This config (mcuboot_demo: demo/mcuboot + boot/mcuboot) |
| `-CleanAll` / `--clean-all` | `demo`, `mcuboot_demo` only | Entire `build/<board>/` then rebuild |

```powershell
.\scripts\ps1\clean.ps1 -All
.\scripts\ps1\clean.ps1 -Board stm3210e_eval
.\scripts\ps1\clean.ps1 demo -Config default
.\scripts\ps1\clean.ps1 sample gpio -Config default
.\scripts\ps1\clean.ps1 boot
```

```bash
./scripts/sh/clean.sh --all
./scripts/sh/clean.sh --board stm3210e_eval
./scripts/sh/clean.sh demo --config default
```

```bat
scripts\cmd\clean.cmd --all
scripts\cmd\clean.cmd --board stm3210e_eval
scripts\cmd\clean.cmd demo --config default
```

## flash / debug

Never rebuild. Select `demo` or `sample <name>` + `-Config` / `--config`, or
omit args to use last build. Needs ST-Link (STM3210E-EVAL: CN9).

```powershell
.\scripts\ps1\flash.ps1 demo -Config default
.\scripts\ps1\flash.ps1 demo -Config mcuboot
.\scripts\ps1\flash.ps1 demo -Config profile
.\scripts\ps1\flash.ps1 sample gpio -Config default
.\scripts\ps1\flash.ps1
.\scripts\ps1\flash.ps1 demo -Config default -Erase
.\scripts\ps1\flash.ps1 demo -Config default -Runner openocd

.\scripts\ps1\debug.ps1 demo -Config default
.\scripts\ps1\debug.ps1 demo -Config mcuboot
.\scripts\ps1\debug.ps1 sample board_bringup -Config default
.\scripts\ps1\debug.ps1
```

```bash
./scripts/sh/flash.sh demo --config default --erase
./scripts/sh/flash.sh demo --config default --runner openocd
./scripts/sh/debug.sh demo --config default
```

```bat
scripts\cmd\flash.cmd demo --config default
scripts\cmd\debug.cmd demo --config default
```

## test (Twister)

Default: board platform, `--build-only` (no on-target run). Optional focus path
under `tests/` (e.g. `platform/smoke`). `-Native` / `--native` → `native_sim`.

```powershell
.\scripts\ps1\test.ps1
.\scripts\ps1\test.ps1 platform/smoke
.\scripts\ps1\test.ps1 -Native
```

```bash
./scripts/sh/test.sh
./scripts/sh/test.sh platform/smoke --native
```

```bat
scripts\cmd\test.cmd
scripts\cmd\test.cmd platform/smoke --native
```

Twister cannot load tests when the repo and west workspace are on **different
Windows drives** (Zephyr `os.path.relpath` limit). Put both on the same drive
or set `ZPLATFORM_WEST_WORKSPACE` accordingly.

## Git Bash on Windows

Prefer `scripts/ps1/` or `scripts/cmd/` day-to-day. For `.sh`:

```bash
# Git Bash
cd /e/Personal/zplatform
./scripts/sh/build.sh demo --board stm3210e_eval
```

```powershell
& "C:\Program Files\Git\bin\bash.exe" -lc "./scripts/sh/build.sh demo --board stm3210e_eval"
```

WSL: `cd /mnt/e/Personal/zplatform` then the same `./scripts/sh/...` commands.

## Smoke-test checklist

| Script | Pass criteria |
|--------|----------------|
| `setup-zephyr` status | Environment READY / VALID |
| `build` demo / sample | Links `zephyr.elf` (Zephyr **4.4.2**, SDK **≥1.0**) |
| `clean` scopes | Removes the expected trees under `build/` |
| `demo` / `mcuboot_demo` (build) | Expected images under `demo/<config>/` |
| `profile` | Profiling build completes |
| `test` | Twister build-only finishes (same-drive workspace) |
| `flash` / `debug` | Needs board + probe |
