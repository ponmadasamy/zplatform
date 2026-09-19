# Scripts layout

```text
scripts/
├── sh/     # Linux / macOS / Git Bash
├── ps1/    # Windows PowerShell
├── cmd/    # Windows cmd.exe (when PowerShell execution is blocked)
└── env/    # Shared Python environment manager (zplatform_env.py)
```

Pick the folder for your host — no wrappers.

## Linux / macOS / Git Bash

```bash
./scripts/sh/setup-zephyr.sh --status
./scripts/sh/setup-zephyr.sh --sync
./scripts/sh/build.sh demo --board stm3210e_eval
./scripts/sh/flash.sh
./scripts/sh/debug.sh
./scripts/sh/profile.sh
./scripts/sh/test.sh
./scripts/sh/mcuboot_demo.sh
```

### Running `.sh` on Windows

Windows does not run bash scripts natively. Use one of:

1. **Git Bash** (recommended if Git for Windows is installed):

```bash
# Open "Git Bash", then:
cd /e/Personal/zplatform
./scripts/sh/build.sh demo --board stm3210e_eval
```

Or from PowerShell / cmd:

```powershell
& "C:\Program Files\Git\bin\bash.exe" -lc "./scripts/sh/build.sh demo --board stm3210e_eval"
```

2. **WSL** (Ubuntu, etc.):

```bash
cd /mnt/e/Personal/zplatform
./scripts/sh/build.sh demo --board stm3210e_eval
```

For day-to-day Windows work prefer `scripts/ps1/` or `scripts/cmd/`.

## Windows PowerShell

```powershell
.\scripts\ps1\setup-zephyr.ps1 -Status
.\scripts\ps1\setup-zephyr.ps1 -Sync
.\scripts\ps1\build.ps1 demo -Board stm3210e_eval
.\scripts\ps1\flash.ps1
.\scripts\ps1\debug.ps1
.\scripts\ps1\profile.ps1
.\scripts\ps1\test.ps1
```

## Windows cmd.exe (PowerShell blocked)

Use when `ExecutionPolicy` blocks `.ps1` scripts:

```bat
scripts\cmd\setup-zephyr.cmd --status
scripts\cmd\setup-zephyr.cmd --sync
scripts\cmd\build.cmd demo --board stm3210e_eval
scripts\cmd\flash.cmd
scripts\cmd\debug.cmd
scripts\cmd\profile.cmd
scripts\cmd\test.cmd
```

Flags match the `sh/` style (`--board`, `--clean`, …).

## Workspace / SDK policy

- Default west workspace: `~/zephyrproject` (override `ZPLATFORM_WEST_WORKSPACE`).
- Compatible SDK is selected only for the current command via scoped env vars.
- Existing SDK installs are kept; never uninstalled.

## Smoke-test checklist

| Script | Pass criteria |
|--------|----------------|
| `setup-zephyr` `--status` / `-Status` | Environment READY |
| `build` demo / sample | Links `zephyr.elf` with Zephyr **4.4.2** + SDK **≥1.0** |
| `profile` | Profiling build completes |
| `test` | Twister build-only finishes |
| `flash` / `debug` | Needs board + probe; skip without hardware |

Use `--clean` / `-Clean` after toolchain changes so CMake does not reuse a stale cache from another Zephyr tree.
