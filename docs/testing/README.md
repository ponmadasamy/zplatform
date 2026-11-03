# Testing

ZPlatform tests live under `tests/` and use **Ztest** + **Twister**.

Samples are not tests. The full demo is not a test.

## Layout

```text
tests/
├── platform/smoke/    # version, system-info, and memory-info smoke tests
└── drivers/
    ├── display/       # MB895 display API tests (STM3210E-EVAL)
    └── dummy/         # placeholder test suite
```

The platform smoke suite is allowed on `stm3210e_eval` and `native_sim`.
The display suite requires the `zephyr,display` chosen node and is allowed on
`stm3210e_eval`. The dummy suite is a Twister-discovery placeholder, not a
driver validation suite. Sample applications and the demo are not part of
these `tests/` runs.

## Commands

```bash
./scripts/sh/test.sh
./scripts/sh/test.sh platform/smoke
./scripts/sh/test.sh drivers/display
./scripts/sh/test.sh --native
```

On Windows, use the matching `scripts/ps1/test.ps1` or
`scripts/cmd/test.cmd` entry point. A focus argument is a path under `tests/`.
Default board mode builds for `stm3210e_eval` only; `--native` selects
`native_sim`. The script adds `--build-only` in board mode, so it does not
execute tests on hardware.

## Result honesty

| Label | Meaning |
|-------|---------|
| BUILD PASS | Firmware / test image compiled |
| ZTEST PASS | Twister executed tests (often on `native_sim`) |
| HARDWARE VERIFIED | Ran on physical target with observed PASS |
| HARDWARE NOT VERIFIED | No physical execution claimed |

Default `./scripts/sh/test.sh` uses `--build-only` for `stm3210e_eval`.
`HARDWARE VERIFIED` should be reported only for a recorded physical run; the
dated hardware-validation notes for this board are in
[`../boards/stm3210e_eval.md`](../boards/stm3210e_eval.md).
