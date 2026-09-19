# ZPlatform

ZPlatform is a scalable, modular platform layer for **Zephyr RTOS**. It hosts
STM32 evaluation kits, custom boards, out-of-tree drivers, middleware, and
platform services without restructuring the repository.

## Zephyr version

| Item | Value |
|------|-------|
| Manifest revision | **v4.4.0** |
| Why | Latest stable with STM32F1, OpenOCD/ST-Link runners, and MCUboot support |

## First hardware target

| Item | Value |
|------|-------|
| Board | STM3210E-EVAL |
| MCU | STM32F103ZGT6 (Cortex-M3) |
| Flash / SRAM | 1024 KiB / 96 KiB |
| Upstream board | **Not present** — ZPlatform provides OOT support |

## Quick start

### West workspace

```bash
mkdir zplatform-ws && cd zplatform-ws
west init -m git@github.com:ponmadasamy/zplatform.git
west update
cd zplatform
```

### Build / flash

```bash
./scripts/build.sh demo --board stm3210e_eval
./scripts/flash.sh

./scripts/build.sh sample board_bringup --board stm3210e_eval
./scripts/flash.sh
```

### Debug / profile / test

```bash
./scripts/debug.sh
./scripts/profile.sh
./scripts/test.sh
```

## Repository layout

```text
zplatform/
├── boards/st/stm3210e_eval/   # OOT board support
├── demo/zplatform_demo/       # full platform demo
├── samples/                   # board-agnostic samples
├── lib/                       # platform services
├── include/zplatform/         # public headers
├── tests/                     # Ztest / Twister
├── scripts/                   # build/flash/debug/profile/test
├── docs/                      # architecture & board docs
├── keys/                      # signing key docs (no private keys)
├── west.yml                   # west manifest
└── zephyr/module.yml          # Zephyr module metadata
```

## Documentation

- [Architecture](docs/architecture/platform.md)
- [STM3210E-EVAL](docs/boards/stm3210e_eval.md)
- [Debugging](docs/debugging/README.md)
- [Testing](docs/testing/README.md)

## License

Apache License 2.0 — see [LICENSE](LICENSE).
