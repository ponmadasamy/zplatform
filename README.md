# ZPlatform

ZPlatform is a scalable, modular platform layer for **Zephyr RTOS**. It hosts
STM32 evaluation kits, custom boards, out-of-tree drivers, middleware, and
platform services without restructuring the repository.

## Zephyr version

| Item | Value |
|------|-------|
| Source of truth | [`config/toolchain.yml`](config/toolchain.yml) |
| Release line | **4.4** |
| Exact version | **4.4.2** |
| Git revision | **v4.4.2** |
| SDK | **≥ 1.0.0** |
| Python | **≥ 3.12** |

Do not edit `west.yml` revision independently — run
`./scripts/sh/setup-zephyr.sh --sync-manifest` (or the `ps1` equivalent).

## Environment manager

```bash
./scripts/sh/setup-zephyr.sh --status
./scripts/sh/setup-zephyr.sh --check-latest
```

```powershell
.\scripts\ps1\setup-zephyr.ps1 -Status
.\scripts\ps1\setup-zephyr.ps1 -CheckLatest
```

Scripts live under `scripts/sh/`, `scripts/ps1/`, and `scripts/cmd/`. See
[scripts/README.md](scripts/README.md) and [docs/environment.md](docs/environment.md).

**Does not uninstall** existing SDK installs (e.g. `zephyr-sdk-0.17.4`).
Default west workspace is `~/zephyrproject` (Zephyr getting-started path);
override with `ZPLATFORM_WEST_WORKSPACE` if needed.

## First hardware target

| Item | Value |
|------|-------|
| Board | STM3210E-EVAL |
| MCU | STM32F103ZGT6 (Cortex-M3) |
| Flash / SRAM | 1024 KiB / 96 KiB |
| Upstream board | **Not present** — ZPlatform provides OOT support |

## Quick start

```bash
git clone git@github.com:ponmadasamy/zplatform.git
cd zplatform
./scripts/sh/build.sh demo --board stm3210e_eval
```

```powershell
.\scripts\ps1\build.ps1 demo -Board stm3210e_eval
```

If PowerShell scripts are blocked (`ExecutionPolicy`), use cmd instead:

```bat
scripts\cmd\build.cmd demo --board stm3210e_eval
```

The build script bootstraps Python `.venv`, west workspace, Zephyr **v4.4.2**,
and a compatible SDK automatically. No manual Zephyr install is required.

### Flash / debug / test

```bash
./scripts/sh/flash.sh
./scripts/sh/debug.sh
./scripts/sh/profile.sh
./scripts/sh/test.sh
```

## Repository layout

```text
zplatform/
├── config/toolchain.yml       # environment lock (authoritative)
├── boards/st/stm3210e_eval/   # OOT board support
├── demo/zplatform_demo/       # full platform demo
├── samples/                   # board-agnostic samples
├── lib/                       # platform services
├── include/zplatform/         # public headers
├── tests/                     # Ztest / Twister
├── scripts/                   # sh/ + ps1/ + cmd/ + env/
├── docs/                      # architecture & board docs
├── keys/                      # signing key docs (no private keys)
├── west.yml                   # west manifest (revision synced from toolchain.yml)
└── zephyr/module.yml          # Zephyr module metadata
```

## Documentation

- [Environment manager](docs/environment.md)
- [Architecture](docs/architecture/platform.md)
- [STM3210E-EVAL](docs/boards/stm3210e_eval.md)
- [Debugging](docs/debugging/README.md)
- [Testing](docs/testing/README.md)

## License

Apache License 2.0 — see [LICENSE](LICENSE).
