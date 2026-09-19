# ZPlatform Environment Manager

The environment manager guarantees **reproducible** Zephyr builds using the
standard Zephyr workspace path (same idea as the SDK install location).

## Single source of truth

```text
config/toolchain.yml
```

`west.yml` **must** match. Sync with:

```bash
./scripts/sh/setup-zephyr.sh --sync-manifest
```

```powershell
.\scripts\ps1\setup-zephyr.ps1 -SyncManifest
```

## Scripts layout

See [scripts/README.md](../scripts/README.md).

| Host | Entry |
|------|-------|
| Linux / macOS / Git Bash | `./scripts/sh/build.sh`, `./scripts/sh/setup-zephyr.sh` |
| Windows PowerShell | `.\scripts\ps1\build.ps1`, `.\scripts\ps1\setup-zephyr.ps1` |
| Windows cmd (PS blocked) | `scripts\cmd\build.cmd`, `scripts\cmd\setup-zephyr.cmd` |

All call `scripts/env/zplatform_env.py`.

## Default workspace (Zephyr getting-started path)

```text
~/zephyrproject/          # default — same as Zephyr docs
├── .west/                # manifest.path = absolute path to this clone
├── zephyr/               # pinned revision from toolchain.yml (e.g. v4.4.2)
└── (modules…)
```

No `zplatform/` junction under the workspace — `.west/config` points at the
clone directly (works across drives, e.g. repo on `E:` and workspace on `C:`).

If `~/zephyrproject` already exists from getting-started (`path = zephyr`),
the first `setup-zephyr --ensure` / `--sync` **adopts** it: points
`.west/config` at this clone and runs `west update` to the pinned revision.

## Override path

```bash
export ZPLATFORM_WEST_WORKSPACE=/path/to/custom-ws
```

```powershell
$env:ZPLATFORM_WEST_WORKSPACE = "D:\path\to\custom-ws"
```

Use this to keep a separate tree (for example the older sibling
`zplatform-ws`) without touching `~/zephyrproject`.

## What is not modified

| Path | Policy |
|------|--------|
| Existing `~/zephyr-sdk-*` installs (e.g. `0.17.4`) | **Kept** — never uninstall / move / overwrite; compatible SDK is selected per command |
| User shell profile (`ZEPHYR_BASE`, `PATH`, …) | **Not changed** — only scoped for the current command |

## SDK selection

```text
Installed SDKs (example):
  0.17.4   ← kept
  1.0.x    ← selected for ZPlatform when present / after install

ZPlatform command:
  ZEPHYR_SDK_INSTALL_DIR=<compatible SDK>   # process-scoped only
```

## Commands

```bash
./scripts/sh/setup-zephyr.sh --status
./scripts/sh/setup-zephyr.sh --ensure
./scripts/sh/setup-zephyr.sh --sync
./scripts/sh/setup-zephyr.sh --check-latest
```

```powershell
.\scripts\ps1\setup-zephyr.ps1 -Status
.\scripts\ps1\setup-zephyr.ps1 -Ensure
.\scripts\ps1\setup-zephyr.ps1 -Sync
.\scripts\ps1\setup-zephyr.ps1 -CheckLatest
```

```bat
scripts\cmd\setup-zephyr.cmd --status
scripts\cmd\setup-zephyr.cmd --ensure
scripts\cmd\setup-zephyr.cmd --sync
scripts\cmd\setup-zephyr.cmd --check-latest
```

## Build entry

```bash
./scripts/sh/build.sh demo --board stm3210e_eval
```

```powershell
.\scripts\ps1\build.ps1 demo -Board stm3210e_eval
```

```bat
scripts\cmd\build.cmd demo --board stm3210e_eval
```

Prints REQUIRED / ACTUAL / SELECTED Zephyr before compile and refuses to
build against the wrong tree.

## Migration notes (4.3 → 4.4)

- Python ≥ 3.12
- Zephyr SDK ≥ 1.0.0
- C17 default
- Workspace default is `~/zephyrproject` (set `ZPLATFORM_WEST_WORKSPACE` if you still use `zplatform-ws`)

See upstream Zephyr 4.4 migration guide.
