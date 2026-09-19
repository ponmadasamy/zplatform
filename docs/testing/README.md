# Testing

ZPlatform tests live under `tests/` and use **Ztest** + **Twister**.

Samples are not tests. The full demo is not a test.

## Layout

```text
tests/
├── platform/     # platform library / integration smoke
└── drivers/      # out-of-tree driver tests (as drivers appear)
```

## Commands

```bash
./scripts/sh/test.sh
./scripts/sh/test.sh platform/smoke
./scripts/sh/test.sh --native
```

## Result honesty

| Label | Meaning |
|-------|---------|
| BUILD PASS | Firmware / test image compiled |
| ZTEST PASS | Twister executed tests (often on `native_sim`) |
| HARDWARE VERIFIED | Ran on physical target with observed PASS |
| HARDWARE NOT VERIFIED | No physical execution claimed |

Default `./scripts/sh/test.sh` uses `--build-only` for `stm3210e_eval`.
