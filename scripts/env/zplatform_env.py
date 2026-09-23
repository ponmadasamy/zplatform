#!/usr/bin/env python3
# Copyright (c) 2026 ZPlatform contributors
# SPDX-License-Identifier: Apache-2.0
"""
ZPlatform environment manager — single cross-platform implementation.

Invoked by setup-zephyr.sh / setup-zephyr.ps1 and by build/test/flash scripts.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import platform
import re
import shutil
import subprocess
import sys
import venv
from dataclasses import dataclass, field, asdict
from pathlib import Path
from typing import Any, Optional

try:
    import yaml
except ImportError:  # bootstrap before requirements install
    yaml = None  # type: ignore


# ---------------------------------------------------------------------------
# Paths / constants
# ---------------------------------------------------------------------------

def repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


def toolchain_path() -> Path:
    return repo_root() / "config" / "toolchain.yml"


def west_yml_path() -> Path:
    return repo_root() / "west.yml"


def venv_dir() -> Path:
    return repo_root() / ".venv"


def venv_python() -> Path:
    if os.name == "nt":
        return venv_dir() / "Scripts" / "python.exe"
    return venv_dir() / "bin" / "python"


def venv_west() -> Path:
    if os.name == "nt":
        return venv_dir() / "Scripts" / "west.exe"
    return venv_dir() / "bin" / "west"


# ---------------------------------------------------------------------------
# Version helpers
# ---------------------------------------------------------------------------

def parse_version(text: str) -> tuple[int, ...]:
    text = text.strip().lstrip("vV")
    parts = re.findall(r"\d+", text)
    if not parts:
        return (0,)
    return tuple(int(p) for p in parts)


def version_ge(have: str, need: str) -> bool:
    return parse_version(have) >= parse_version(need)


def version_str(v: tuple[int, ...]) -> str:
    return ".".join(str(x) for x in v)


# ---------------------------------------------------------------------------
# Config load / west.yml sync
# ---------------------------------------------------------------------------

def load_toolchain() -> dict[str, Any]:
    path = toolchain_path()
    if not path.is_file():
        die(f"missing toolchain config: {path}")
    text = path.read_text(encoding="utf-8")
    if yaml is None:
        return _parse_toolchain_minimal(text)
    data = yaml.safe_load(text)
    if not isinstance(data, dict):
        die("toolchain.yml must be a mapping")
    return data


def _parse_toolchain_minimal(text: str) -> dict[str, Any]:
    """Stdlib fallback for bootstrap before PyYAML is installed."""
    data: dict[str, Any] = {}
    section: Optional[str] = None
    for raw in text.splitlines():
        line = raw.split("#", 1)[0].rstrip()
        if not line.strip():
            continue
        if re.match(r"^[A-Za-z0-9_]+:\s*$", line):
            section = line.strip().rstrip(":")
            data[section] = {}
            continue
        m = re.match(r"^\s+([A-Za-z0-9_]+):\s*(.*)$", line)
        if m and section:
            key, val = m.group(1), m.group(2).strip().strip('"').strip("'")
            if val in ("{}", ""):
                data[section][key] = {}
            else:
                data[section][key] = val
    return data


def required_zephyr_revision(cfg: dict[str, Any]) -> str:
    return str(cfg["zephyr"]["revision"])


def required_zephyr_version(cfg: dict[str, Any]) -> str:
    return str(cfg["zephyr"]["version"])


def required_sdk_min(cfg: dict[str, Any]) -> str:
    return str(cfg["sdk"]["minimum_version"])


def required_python_min(cfg: dict[str, Any]) -> str:
    return str(cfg["python"]["minimum_version"])


def required_cmake_min(cfg: dict[str, Any]) -> str:
    return str(cfg.get("cmake", {}).get("minimum_version", "3.28.0"))


def default_board(cfg: dict[str, Any]) -> str:
    return str(cfg.get("target", {}).get("default_board", "stm3210e_eval"))


def workspace_name(cfg: dict[str, Any]) -> str:
    """Default west workspace directory name under $HOME (Zephyr getting-started)."""
    return str(
        cfg.get("workspace", {}).get("default_name")
        or cfg.get("workspace", {}).get("upstream_name")
        or cfg.get("workspace", {}).get("name")
        or "zephyrproject"
    )


def legacy_isolated_workspace_name(cfg: dict[str, Any]) -> str:
    """Former sibling workspace name — still discovered if already initialized."""
    return str(cfg.get("workspace", {}).get("isolated_name", "zplatform-ws"))


def read_west_yml_revision() -> Optional[str]:
    text = west_yml_path().read_text(encoding="utf-8")
    m = re.search(r"^\s*revision:\s*(\S+)", text, re.MULTILINE)
    return m.group(1) if m else None


def sync_west_manifest(cfg: dict[str, Any]) -> None:
    rev = required_zephyr_revision(cfg)
    path = west_yml_path()
    text = path.read_text(encoding="utf-8")
    new, n = re.subn(
        r"(^\s*revision:\s*)\S+",
        rf"\g<1>{rev}",
        text,
        count=1,
        flags=re.MULTILINE,
    )
    if n != 1:
        die("failed to update west.yml revision (pattern not found)")
    path.write_text(new, encoding="utf-8")
    info(f"Synced west.yml revision -> {rev}")


def validate_manifest_agreement(cfg: dict[str, Any]) -> tuple[bool, str]:
    want = required_zephyr_revision(cfg)
    have = read_west_yml_revision()
    if have != want:
        return False, f"toolchain.yml={want} west.yml={have}"
    return True, want


# ---------------------------------------------------------------------------
# Logging
# ---------------------------------------------------------------------------

def info(msg: str) -> None:
    # Always stderr so --print-env can keep stdout JSON-clean.
    print(msg, file=sys.stderr)


def warn(msg: str) -> None:
    print(f"WARNING: {msg}", file=sys.stderr)


def die(msg: str, code: int = 1) -> None:
    print(f"ERROR: {msg}", file=sys.stderr)
    raise SystemExit(code)


# ---------------------------------------------------------------------------
# Process helpers
# ---------------------------------------------------------------------------

# When True, child process stdout is redirected to stderr (for --print-env).
_REDIRECT_CHILD_STDOUT = False


def run(
    cmd: list[str],
    *,
    cwd: Optional[Path] = None,
    env: Optional[dict[str, str]] = None,
    check: bool = True,
    capture: bool = False,
) -> subprocess.CompletedProcess[str]:
    kwargs: dict[str, Any] = {
        "cwd": str(cwd) if cwd else None,
        "env": env,
        "check": check,
        "text": True,
        "capture_output": capture,
    }
    if _REDIRECT_CHILD_STDOUT and not capture:
        kwargs["stdout"] = sys.stderr
    return subprocess.run(cmd, **kwargs)


def which(name: str) -> Optional[str]:
    return shutil.which(name)


# ---------------------------------------------------------------------------
# Python / venv
# ---------------------------------------------------------------------------

def host_python_version() -> str:
    return f"{sys.version_info.major}.{sys.version_info.minor}.{sys.version_info.micro}"


def ensure_host_python(cfg: dict[str, Any]) -> None:
    need = required_python_min(cfg)
    have = host_python_version()
    if not version_ge(have, need):
        die(
            f"Required Python : >= {need}\n"
            f"Detected Python : {have}\n"
            f"Environment status: INCOMPATIBLE\n"
            f"Remediation: install Python {need}+ and re-run "
            f"./scripts/sh/setup-zephyr.sh (do not replace system Python; "
            f"use an official installer or pyenv)."
        )


def ensure_venv(cfg: dict[str, Any]) -> Path:
    ensure_host_python(cfg)
    py = venv_python()
    created = False
    if not py.is_file():
        info(f"Creating project venv at {venv_dir()}")
        venv.EnvBuilder(with_pip=True, clear=False).create(venv_dir())
        created = True

    # Only re-run pip when the venv is new or requirements.txt changed.
    # Avoids "Requirement already satisfied" spam on every build/flash.
    req = repo_root() / "requirements.txt"
    stamp = venv_dir() / ".zplatform-requirements.sha256"
    digest = ""
    if req.is_file():
        digest = hashlib.sha256(req.read_bytes()).hexdigest()
    need_pip = created or not stamp.is_file() or stamp.read_text(encoding="utf-8").strip() != digest
    if need_pip:
        info("Installing/updating project venv requirements")
        run([str(py), "-m", "pip", "install", "-q", "--upgrade", "pip"], check=True)
        if req.is_file():
            run([str(py), "-m", "pip", "install", "-q", "-r", str(req)], check=True)
        stamp.write_text(digest + "\n", encoding="utf-8")
    return py


def validate_west_in_venv() -> tuple[bool, str]:
    west = venv_west()
    py = venv_python()
    if not west.is_file():
        return False, f"west missing in venv ({west})"
    # Ensure west resolves via venv python
    try:
        out = run(
            [str(py), "-c", "import west; import west.version; print(west.version.__version__)"],
            capture=True,
            check=True,
        )
        ver = out.stdout.strip()
        return True, f"{west} (west {ver})"
    except subprocess.CalledProcessError:
        return False, "west import failed in project venv"


# ---------------------------------------------------------------------------
# Workspace
# ---------------------------------------------------------------------------

def preferred_workspace(cfg: dict[str, Any]) -> Path:
    """
    Default ZPlatform west workspace.

    Same location as Zephyr getting-started: ~/zephyrproject
    Override with ZPLATFORM_WEST_WORKSPACE when a different path is required.
    """
    override = os.environ.get("ZPLATFORM_WEST_WORKSPACE")
    if override:
        return Path(override).expanduser().resolve()
    return Path.home() / workspace_name(cfg)


def upstream_zephyr_workspace(cfg: dict[str, Any]) -> Path:
    """Default getting-started path under $HOME (may be adopted by ZPlatform)."""
    return Path.home() / workspace_name(cfg)


def is_zephyr_manifest_workspace(ws: Path) -> bool:
    """True when .west/config manifest path is upstream 'zephyr' (getting-started)."""
    mpath = west_config_manifest_path(ws)
    if not mpath:
        return False
    name = Path(mpath).name.replace("\\", "/").split("/")[-1]
    return name == "zephyr"


def west_config_manifest_path(ws: Path) -> Optional[str]:
    cfg = ws / ".west" / "config"
    if not cfg.is_file():
        return None
    text = cfg.read_text(encoding="utf-8")
    m = re.search(r"^\s*path\s*=\s*(.+)$", text, re.MULTILINE)
    return m.group(1).strip() if m else None


def workspace_owns_zplatform(ws: Path) -> bool:
    """True if this west workspace's manifest is this ZPlatform clone."""
    root = repo_root().resolve()
    mpath = west_config_manifest_path(ws)
    if not mpath:
        return False
    cand = Path(mpath)
    if not cand.is_absolute():
        cand = (ws / cand).resolve()
    else:
        cand = cand.resolve()
    try:
        return cand == root
    except OSError:
        return False


def find_west_workspace(cfg: dict[str, Any]) -> Optional[Path]:
    """Return preferred workspace if it already owns this ZPlatform clone."""
    ws = preferred_workspace(cfg)
    if (ws / ".west").is_dir() and workspace_owns_zplatform(ws):
        return ws
    return None


def remove_obsolete_zplatform_link(workspace: Path) -> None:
    """Remove workspace/zplatform junction/symlink if it points at this repo."""
    target = workspace / "zplatform"
    root = repo_root().resolve()
    try:
        if not target.exists() and not target.is_symlink():
            return
        if target.resolve() != root:
            return
    except OSError:
        return
    info(f"Removing obsolete zplatform link: {target}")
    try:
        if target.is_symlink():
            target.unlink()
        else:
            target.rmdir()  # Windows directory junction
    except OSError as exc:
        warn(f"Could not remove {target}: {exc}")


def write_west_config(workspace: Path, *, force: bool = False) -> None:
    """
    Create/update .west/config for a ZPlatform-owned workspace.

    manifest.path is an absolute path to this clone — no junction/symlink
    under the workspace is required (important when the clone and
    ~/zephyrproject are on different drives).
    """
    west_dir = workspace / ".west"
    west_dir.mkdir(parents=True, exist_ok=True)
    cfg_path = west_dir / "config"
    manifest = repo_root().resolve().as_posix()
    content = (
        "[manifest]\n"
        f"path = {manifest}\n"
        "file = west.yml\n"
        "\n"
        "[zephyr]\n"
        "base = zephyr\n"
    )

    if cfg_path.is_file() and not force:
        existing = cfg_path.read_text(encoding="utf-8")
        existing_norm = existing.replace("\\", "/")
        # Upgrade old relative "path = zplatform" (junction layout) to absolute.
        if re.search(r"(?m)^\s*path\s*=\s*zplatform\s*$", existing):
            force = True
        elif manifest not in existing_norm:
            # Different clone path or still zephyr manifest — rewrite when forced by caller
            pass
        else:
            remove_obsolete_zplatform_link(workspace)
            return

    if cfg_path.is_file() and not force:
        remove_obsolete_zplatform_link(workspace)
        return

    cfg_path.write_text(content, encoding="utf-8")
    remove_obsolete_zplatform_link(workspace)


def adopt_getting_started_workspace(ws: Path) -> None:
    """
    Convert a classic getting-started workspace (manifest=zephyr) to ZPlatform
    ownership. Zephyr tree is kept and updated via west; no zplatform link.
    """
    info(
        f"Adopting Zephyr workspace {ws} for ZPlatform "
        f"(manifest.path -> {repo_root().resolve()})"
    )
    write_west_config(ws, force=True)


def ensure_workspace(cfg: dict[str, Any], *, sync: bool = False) -> Path:
    """
    Ensure the west workspace at preferred_workspace (~/zephyrproject by default,
    or ZPLATFORM_WEST_WORKSPACE).
    """
    ws = preferred_workspace(cfg)
    legacy = repo_root().parent / legacy_isolated_workspace_name(cfg)
    if (
        not os.environ.get("ZPLATFORM_WEST_WORKSPACE")
        and legacy.is_dir()
        and (legacy / ".west").is_dir()
        and workspace_owns_zplatform(legacy)
        and ws.resolve() != legacy.resolve()
    ):
        info(
            f"Note: legacy workspace {legacy} still exists. "
            f"Default is now {ws}. Set ZPLATFORM_WEST_WORKSPACE={legacy} "
            f"to keep using the legacy path."
        )

    if find_west_workspace(cfg) is not None:
        info(f"Using ZPlatform west workspace: {ws}")
        write_west_config(ws)
    else:
        ws.mkdir(parents=True, exist_ok=True)
        if is_zephyr_manifest_workspace(ws):
            adopt_getting_started_workspace(ws)
        else:
            if not (ws / ".west").is_dir():
                info(f"Initializing ZPlatform west workspace at {ws}")
            write_west_config(ws)

    if sync or not (ws / "zephyr").is_dir():
        info(f"Running west update in {ws} (pinned {required_zephyr_revision(cfg)})")
        run([str(venv_west()), "update"], cwd=ws, check=True)
        zephyr_req = ws / "zephyr" / "scripts" / "requirements.txt"
        if zephyr_req.is_file():
            run(
                [str(venv_python()), "-m", "pip", "install", "-q", "-r", str(zephyr_req)],
                check=False,
            )
    return ws


# ---------------------------------------------------------------------------
# Zephyr revision detection
# ---------------------------------------------------------------------------

@dataclass
class ZephyrInfo:
    base: Optional[str] = None
    version: Optional[str] = None
    git_describe: Optional[str] = None
    git_exact_tag: Optional[str] = None
    west_revision: Optional[str] = None
    ok: bool = False
    detail: str = ""


def read_zephyr_version_file(base: Path) -> Optional[str]:
    vf = base / "VERSION"
    if not vf.is_file():
        return None
    vals: dict[str, str] = {}
    for line in vf.read_text(encoding="utf-8").splitlines():
        if "=" in line:
            k, v = line.split("=", 1)
            vals[k.strip()] = v.strip()
    try:
        maj = vals.get("VERSION_MAJOR", "0")
        minor = vals.get("VERSION_MINOR", "0")
        patch = vals.get("PATCHLEVEL", "0")
        extra = vals.get("EXTRAVERSION", "")
        ver = f"{maj}.{minor}.{patch}"
        if extra:
            ver = f"{ver}-{extra}" if not extra.startswith("-") else f"{ver}{extra}"
        return ver
    except Exception:
        return None


def git_describe(path: Path) -> Optional[str]:
    if not (path / ".git").exists() and not (path / ".git").is_file():
        # west projects may use .git files
        pass
    try:
        out = run(
            ["git", "describe", "--tags", "--always"],
            cwd=path,
            capture=True,
            check=True,
        )
        return out.stdout.strip()
    except (subprocess.CalledProcessError, FileNotFoundError):
        return None


def git_exact_tag(path: Path) -> Optional[str]:
    try:
        out = run(
            ["git", "describe", "--tags", "--exact-match"],
            cwd=path,
            capture=True,
            check=False,
        )
        if out.returncode == 0:
            return out.stdout.strip()
    except FileNotFoundError:
        return None
    return None


def inspect_zephyr(ws: Path, cfg: dict[str, Any]) -> ZephyrInfo:
    info_obj = ZephyrInfo()
    base = ws / "zephyr"
    if not base.is_dir():
        info_obj.detail = f"zephyr missing under {ws}"
        return info_obj
    info_obj.base = str(base.resolve())
    info_obj.version = read_zephyr_version_file(base)
    info_obj.git_describe = git_describe(base)
    info_obj.git_exact_tag = git_exact_tag(base)
    info_obj.west_revision = read_west_yml_revision()

    required = required_zephyr_revision(cfg)
    want_ver = required_zephyr_version(cfg)
    have_ver = (info_obj.version or "").split("-")[0]

    if have_ver != want_ver:
        info_obj.ok = False
        info_obj.detail = (
            f"ZPlatform requires:\n    Zephyr {required}\n"
            f"Detected:\n    Zephyr {info_obj.version or 'unknown'} "
            f"({info_obj.git_describe or 'no git describe'})\n"
            f"Result:\n    VERSION MISMATCH\n"
            f"Run:\n    ./scripts/sh/setup-zephyr.sh --sync"
        )
        return info_obj

    # VERSION matches. Prefer exact tag equality when available.
    if info_obj.git_exact_tag and info_obj.git_exact_tag != required:
        info_obj.ok = False
        info_obj.detail = (
            f"ERROR: Zephyr revision mismatch\n\n"
            f"Required : {required}\n"
            f"Workspace: {info_obj.git_exact_tag}\n\n"
            f"Run:\n    ./scripts/sh/setup-zephyr.sh --sync"
        )
        return info_obj

    info_obj.ok = True
    info_obj.detail = "match"
    return info_obj


# ---------------------------------------------------------------------------
# SDK discovery / selection
# ---------------------------------------------------------------------------

@dataclass
class SdkInfo:
    path: Optional[str] = None
    version: Optional[str] = None
    candidates: list[str] = field(default_factory=list)
    ok: bool = False
    detail: str = ""


def discover_sdk_roots() -> list[Path]:
    homes = [Path.home()]
    env = os.environ.get("ZEPHYR_SDK_INSTALL_DIR")
    roots: list[Path] = []
    if env:
        roots.append(Path(env))
    for home in homes:
        for p in home.glob("zephyr-sdk-*"):
            if p.is_dir():
                roots.append(p)
        # Nested under zephyrproject
        zp = home / "zephyrproject"
        if zp.is_dir():
            for p in zp.glob("zephyr-sdk-*"):
                if p.is_dir():
                    roots.append(p)
    # Deduplicate
    uniq: list[Path] = []
    seen = set()
    for r in roots:
        key = str(r.resolve()) if r.exists() else str(r)
        if key not in seen:
            seen.add(key)
            uniq.append(r)
    return uniq


def sdk_version_of(path: Path) -> Optional[str]:
    # Prefer sdk_version file / VERSION
    for name in ("sdk_version", "VERSION", "version"):
        f = path / name
        if f.is_file():
            text = f.read_text(encoding="utf-8", errors="ignore").strip()
            m = re.search(r"\d+\.\d+(?:\.\d+)?", text)
            if m:
                return m.group(0)
    m = re.search(r"zephyr-sdk-(\d+\.\d+(?:\.\d+)?)", path.name)
    if m:
        return m.group(1)
    return None


def select_sdk(cfg: dict[str, Any]) -> SdkInfo:
    need = required_sdk_min(cfg)
    info_obj = SdkInfo()
    candidates = []
    compatible: list[tuple[tuple[int, ...], Path, str]] = []
    for root in discover_sdk_roots():
        ver = sdk_version_of(root)
        if not ver:
            continue
        candidates.append(f"{ver} ({root})")
        if version_ge(ver, need):
            compatible.append((parse_version(ver), root, ver))
    info_obj.candidates = candidates
    if not compatible:
        info_obj.ok = False
        info_obj.detail = (
            f"Required:\n  SDK : >= {need}\n"
            f"Detected:\n  {candidates or ['(none)']}\n"
            f"Status:\n  INCOMPATIBLE\n"
            f"Install with: west sdk install (from workspace zephyr/) "
            f"or download Zephyr SDK >= {need}. Old SDKs are left installed."
        )
        return info_obj
    compatible.sort(reverse=True)
    _, path, ver = compatible[0]
    info_obj.path = str(path.resolve())
    info_obj.version = ver
    info_obj.ok = True
    info_obj.detail = f"selected {ver} from {len(candidates)} installed"
    return info_obj


def try_install_sdk(ws: Path, cfg: dict[str, Any]) -> SdkInfo:
    """
    Install a compatible Zephyr SDK alongside any existing SDKs.

    Never uninstalls, moves, or overwrites older SDKs (e.g. 0.17.x used by
    the original ~/zephyrproject). Selection for ZPlatform is scoped via
    ZEPHYR_SDK_INSTALL_DIR for the current command only.
    """
    zephyr = ws / "zephyr"
    if not zephyr.is_dir():
        return select_sdk(cfg)
    info(
        f"Installing Zephyr SDK >= {required_sdk_min(cfg)} via 'west sdk install' "
        f"(existing SDKs under $HOME/zephyr-sdk-* are left unchanged)"
    )
    sdk_ver_file = zephyr / "SDK_VERSION"
    args = [str(venv_west()), "sdk", "install"]
    if sdk_ver_file.is_file():
        ver = sdk_ver_file.read_text(encoding="utf-8").strip()
        if ver:
            args += ["--version", ver]
    # Only ARM Cortex toolchains needed for STM32 — avoid downloading every arch.
    args += ["-t", "arm-zephyr-eabi"]
    try:
        run(args, cwd=zephyr, check=False)
    except FileNotFoundError:
        pass
    return select_sdk(cfg)


# ---------------------------------------------------------------------------
# Host tools
# ---------------------------------------------------------------------------

@dataclass
class ToolStatus:
    name: str
    required: bool
    ok: bool
    detail: str = ""
    path: Optional[str] = None
    version: Optional[str] = None


def check_cmake(cfg: dict[str, Any]) -> ToolStatus:
    path = which("cmake")
    if not path:
        return ToolStatus("cmake", True, False, "not found")
    try:
        out = run([path, "--version"], capture=True, check=True)
        first = out.stdout.splitlines()[0]
        m = re.search(r"(\d+\.\d+\.\d+)", first)
        ver = m.group(1) if m else "unknown"
        ok = version_ge(ver, required_cmake_min(cfg)) if ver != "unknown" else True
        return ToolStatus("cmake", True, ok, first, path, ver)
    except subprocess.CalledProcessError:
        return ToolStatus("cmake", True, False, "version check failed", path)


def check_ninja() -> ToolStatus:
    path = which("ninja") or which("ninja.exe")
    if not path:
        return ToolStatus("ninja", True, False, "not found")
    try:
        out = run([path, "--version"], capture=True, check=True)
        ver = out.stdout.strip()
        return ToolStatus("ninja", True, True, ver, path, ver)
    except subprocess.CalledProcessError:
        return ToolStatus("ninja", True, False, "failed", path)


def check_git() -> ToolStatus:
    path = which("git")
    if not path:
        return ToolStatus("git", True, False, "not found")
    out = run([path, "--version"], capture=True, check=True)
    return ToolStatus("git", True, True, out.stdout.strip(), path)


def check_arm_gcc(sdk: SdkInfo) -> ToolStatus:
    candidates: list[Path] = []
    if sdk.path:
        root = Path(sdk.path)
        # Zephyr SDK 0.x layout and SDK 1.x (gnu/) layout
        for rel in (
            Path("arm-zephyr-eabi") / "bin" / ("arm-zephyr-eabi-gcc.exe" if os.name == "nt" else "arm-zephyr-eabi-gcc"),
            Path("gnu") / "arm-zephyr-eabi" / "bin" / ("arm-zephyr-eabi-gcc.exe" if os.name == "nt" else "arm-zephyr-eabi-gcc"),
        ):
            candidates.append(root / rel)
    w = which("arm-zephyr-eabi-gcc")
    if w:
        candidates.append(Path(w))
    for c in candidates:
        if c.is_file():
            try:
                out = run([str(c), "--version"], capture=True, check=True)
                ver = out.stdout.splitlines()[0]
                return ToolStatus("arm-gcc", True, True, ver, str(c))
            except subprocess.CalledProcessError:
                continue
    return ToolStatus("arm-gcc", True, False, "arm-zephyr-eabi-gcc not found")


def check_gdb(sdk: SdkInfo) -> ToolStatus:
    if sdk.path:
        root = Path(sdk.path)
        for rel in (
            Path("arm-zephyr-eabi") / "bin" / ("arm-zephyr-eabi-gdb.exe" if os.name == "nt" else "arm-zephyr-eabi-gdb"),
            Path("gnu") / "arm-zephyr-eabi" / "bin" / ("arm-zephyr-eabi-gdb.exe" if os.name == "nt" else "arm-zephyr-eabi-gdb"),
        ):
            p = root / rel
            if p.is_file():
                return ToolStatus("gdb", False, True, "found", str(p))
    w = which("arm-zephyr-eabi-gdb") or which("gdb-multiarch") or which("gdb")
    if w:
        return ToolStatus("gdb", False, True, "found", w)
    return ToolStatus("gdb", False, False, "not found (needed for debug)")


def check_flash_tools() -> dict[str, ToolStatus]:
    return {
        "openocd": ToolStatus(
            "openocd", False, bool(which("openocd")),
            "found" if which("openocd") else "not found",
            which("openocd"),
        ),
        "st-flash": ToolStatus(
            "st-flash", False, bool(which("st-flash")),
            "found" if which("st-flash") else "not found",
            which("st-flash"),
        ),
        "STM32_Programmer_CLI": ToolStatus(
            "STM32_Programmer_CLI", False,
            bool(which("STM32_Programmer_CLI")),
            "found" if which("STM32_Programmer_CLI") else "not found",
            which("STM32_Programmer_CLI"),
        ),
        "JLinkExe": ToolStatus(
            "JLinkExe", False,
            bool(which("JLinkExe") or which("JLink.exe")),
            "found" if (which("JLinkExe") or which("JLink.exe")) else "not found",
            which("JLinkExe") or which("JLink.exe"),
        ),
    }


# ---------------------------------------------------------------------------
# Environment bundle
# ---------------------------------------------------------------------------

@dataclass
class EnvReport:
    ready: bool
    cfg_revision: str
    cfg_version: str
    python: str
    python_path: str
    west: str
    west_ok: bool
    workspace: Optional[str]
    zephyr: ZephyrInfo
    sdk: SdkInfo
    cmake: ToolStatus
    ninja: ToolStatus
    git: ToolStatus
    arm_gcc: ToolStatus
    gdb: ToolStatus
    flash_tools: dict[str, ToolStatus]
    manifest_ok: bool
    manifest_detail: str
    messages: list[str] = field(default_factory=list)


def build_scoped_env(ws: Path, sdk: SdkInfo) -> dict[str, str]:
    """
    Per-command environment only. Does not modify the user's shell profile or
    permanently change ZEPHYR_BASE / ZEPHYR_SDK_INSTALL_DIR for other projects.
    """
    env = os.environ.copy()
    # Override ambient ZEPHYR_BASE for this process so an old ~/zephyrproject
    # checkout (e.g. 4.3.99) is never used by accident.
    env["ZEPHYR_BASE"] = str((ws / "zephyr").resolve())
    env["ZEPHYR_TOOLCHAIN_VARIANT"] = "zephyr"
    if sdk.path:
        env["ZEPHYR_SDK_INSTALL_DIR"] = sdk.path
    vbindir = str(venv_python().parent)
    env["PATH"] = vbindir + os.pathsep + env.get("PATH", "")
    env["VIRTUAL_ENV"] = str(venv_dir())
    env["ZPLATFORM_ROOT"] = str(repo_root())
    env["ZPLATFORM_WEST_WORKSPACE"] = str(ws.resolve())
    return env


def gather_report(cfg: dict[str, Any], *, require_zephyr: bool = True) -> EnvReport:
    msgs: list[str] = []
    py_path = str(venv_python()) if venv_python().is_file() else sys.executable
    west_ok, west_detail = validate_west_in_venv() if venv_west().is_file() else (False, "venv west missing")
    manifest_ok, manifest_detail = validate_manifest_agreement(cfg)

    ws = preferred_workspace(cfg)
    zephyr = ZephyrInfo(detail="workspace not initialized")
    sdk = select_sdk(cfg)
    if (ws / "zephyr").is_dir():
        zephyr = inspect_zephyr(ws, cfg)

    cmake = check_cmake(cfg)
    ninja = check_ninja()
    git = check_git()
    arm = check_arm_gcc(sdk)
    gdb = check_gdb(sdk)
    flash = check_flash_tools()

    ready = True
    if not version_ge(host_python_version(), required_python_min(cfg)):
        ready = False
        msgs.append("Python INCOMPATIBLE")
    if not west_ok:
        ready = False
        msgs.append("west INCOMPATIBLE")
    if not manifest_ok:
        ready = False
        msgs.append(f"manifest mismatch: {manifest_detail}")
    if require_zephyr:
        if not zephyr.ok:
            ready = False
            msgs.append("Zephyr VERSION MISMATCH")
        if not sdk.ok:
            ready = False
            msgs.append("SDK INCOMPATIBLE")
        if not cmake.ok or not ninja.ok or not git.ok or not arm.ok:
            ready = False
            msgs.append("host/toolchain tool failure")

    return EnvReport(
        ready=ready,
        cfg_revision=required_zephyr_revision(cfg),
        cfg_version=required_zephyr_version(cfg),
        python=host_python_version(),
        python_path=py_path,
        west=west_detail,
        west_ok=west_ok,
        workspace=str(ws) if ws else None,
        zephyr=zephyr,
        sdk=sdk,
        cmake=cmake,
        ninja=ninja,
        git=git,
        arm_gcc=arm,
        gdb=gdb,
        flash_tools=flash,
        manifest_ok=manifest_ok,
        manifest_detail=manifest_detail,
        messages=msgs,
    )


def print_status(report: EnvReport, cfg: dict[str, Any]) -> None:
    def st(ok: bool) -> str:
        return "PASS" if ok else "FAIL"

    print("========================================")
    print("ZPlatform Environment")
    print("========================================")
    print()
    print("Host:")
    print(f"  OS           : {platform.system()}")
    print(f"  Architecture : {platform.machine()}")
    print()
    print("Python:")
    print(f"  Required     : >= {required_python_min(cfg)}")
    print(f"  Detected     : {report.python}")
    print(f"  Path         : {report.python_path}")
    print(f"  Status       : {st(version_ge(report.python, required_python_min(cfg)))}")
    print()
    print("West:")
    print(f"  Detail       : {report.west}")
    print(f"  Status       : {st(report.west_ok)}")
    print()
    print("Manifest:")
    print(f"  toolchain.yml: {report.cfg_revision}")
    print(f"  west.yml     : {read_west_yml_revision()}")
    print(f"  Status       : {st(report.manifest_ok)}")
    print()
    print("Zephyr:")
    print(f"  Required     : {report.cfg_revision} ({report.cfg_version})")
    print(f"  Detected     : {report.zephyr.version or 'n/a'}")
    print(f"  Git          : {report.zephyr.git_describe or 'n/a'}")
    print(f"  Exact tag    : {report.zephyr.git_exact_tag or 'n/a'}")
    print(f"  ZEPHYR_BASE  : {report.zephyr.base or 'n/a'}")
    print(f"  Workspace    : {report.workspace or 'n/a'}")
    print(f"  Status       : {st(report.zephyr.ok)}")
    if report.zephyr.detail and not report.zephyr.ok:
        print(f"  Detail       : {report.zephyr.detail}")
    print()
    print("Zephyr SDK:")
    print(f"  Required     : >= {required_sdk_min(cfg)}")
    print(f"  Detected     : {report.sdk.version or 'n/a'}")
    print(f"  Path         : {report.sdk.path or 'n/a'}")
    if report.sdk.candidates:
        print("  Installed    :")
        for c in report.sdk.candidates:
            print(f"    - {c}")
    print(f"  Status       : {st(report.sdk.ok)}")
    print()
    print("CMake:")
    print(f"  {report.cmake.detail} [{st(report.cmake.ok)}]")
    print("Ninja:")
    print(f"  {report.ninja.detail} [{st(report.ninja.ok)}]")
    print("Git:")
    print(f"  {report.git.detail} [{st(report.git.ok)}]")
    print("ARM toolchain:")
    print(f"  {report.arm_gcc.detail} [{st(report.arm_gcc.ok)}]")
    print("GDB:")
    print(f"  {report.gdb.detail} [{st(report.gdb.ok)}]")
    print()
    print("Flash/debug tools (optional for build):")
    for name, t in report.flash_tools.items():
        print(f"  {name:22}: {t.detail}")
    print()
    print("Environment:")
    print(f"  {'READY' if report.ready else 'NOT READY'}")
    print("========================================")


def print_build_banner(
    report: EnvReport,
    *,
    board: str,
    application: str,
    config: str,
) -> None:
    print("========================================")
    print("ZPlatform Build")
    print("========================================")
    print()
    print(f"Board       : {board}")
    print(f"Application : {application}")
    print(f"Config      : {config}")
    print()
    print(f"REQUIRED Zephyr : {report.cfg_revision}")
    print(f"ACTUAL Zephyr   : {report.zephyr.version or 'n/a'} ({report.zephyr.git_describe or 'n/a'})")
    print(f"SELECTED Zephyr : {report.zephyr.base or 'n/a'}")
    print(f"SDK             : {report.sdk.version or 'n/a'}")
    print(f"Python          : {report.python}")
    print(f"west            : {report.west}")
    print()
    print(f"Environment : {'VALID' if report.ready else 'INVALID'}")
    print("========================================")
    print()


def _fmt_size(n: int) -> str:
    if n >= 1024 * 1024 and n % (1024 * 1024) == 0:
        return f"{n // (1024 * 1024)} MiB"
    if n >= 1024 and n % 1024 == 0:
        return f"{n // 1024} KiB"
    return f"{n} B"


def print_flash_layout(build_dir: Path) -> int:
    """Print linker FLASH region + DTS fixed-partitions after a west build.

    Sourced from the build tree (zephyr/linker.cmd, .config, zephyr.dts) so the
    summary matches what was actually linked — default vs -Mcuboot differ here.
    """
    build_dir = Path(build_dir)
    zephyr = build_dir / "zephyr"
    linker = zephyr / "linker.cmd"
    config = zephyr / ".config"
    dts = zephyr / "zephyr.dts"

    if not zephyr.is_dir():
        print(f"error: no zephyr/ under {build_dir}", file=sys.stderr)
        return 1

    print()
    print("Flash / code layout (from this build)")
    print("-------------------------------------")

    # Linker MEMORY FLASH line (what the "Memory region FLASH" summary uses).
    if linker.is_file():
        text = linker.read_text(encoding="utf-8", errors="replace")
        m = re.search(
            r"FLASH\s*\([^)]*\)\s*:\s*ORIGIN\s*=\s*\(([^)]+)\),\s*LENGTH\s*=\s*\(([^)]+)\)",
            text,
        )
        if m:
            print(f"  Linker FLASH : ORIGIN = ({m.group(1).strip()})")
            print(f"                 LENGTH = ({m.group(2).strip()})")
        else:
            m2 = re.search(r"FLASH\s*\([^)]*\)\s*:\s*ORIGIN\s*=\s*([^,]+),\s*LENGTH\s*=\s*([^\n]+)", text)
            if m2:
                print(f"  Linker FLASH : ORIGIN = {m2.group(1).strip()}")
                print(f"                 LENGTH = {m2.group(2).strip()}")

    load_off = load_sz = rom_off = None
    if config.is_file():
        cfg_txt = config.read_text(encoding="utf-8", errors="replace")

        def _cfg_int(name: str) -> Optional[int]:
            mm = re.search(rf"^CONFIG_{name}=(0x[0-9a-fA-F]+|\d+)\s*$", cfg_txt, re.M)
            if not mm:
                return None
            return int(mm.group(1), 0)

        load_off = _cfg_int("FLASH_LOAD_OFFSET")
        load_sz = _cfg_int("FLASH_LOAD_SIZE")
        rom_off = _cfg_int("ROM_START_OFFSET")
        if load_off is not None:
            print(f"  LOAD_OFFSET  : 0x{load_off:X}  (image linked at flash base + offset)")
        if load_sz is not None and load_sz > 0:
            print(f"  LOAD_SIZE    : 0x{load_sz:X} ({_fmt_size(load_sz)})  (max image for this link)")
        elif load_sz == 0:
            print("  LOAD_SIZE    : 0 (use full remaining flash - typical without code-partition)")
        if rom_off is not None:
            print(f"  ROM_START_OFF: 0x{rom_off:X}  (MCUboot header reserve when non-zero)")

    code_part = None
    if dts.is_file():
        dts_txt = dts.read_text(encoding="utf-8", errors="replace")
        cm = re.search(r"zephyr,code-partition\s*=\s*&([A-Za-z0-9_]+)\s*;", dts_txt)
        if cm:
            code_part = cm.group(1)
            print(f"  code-partition: &{code_part}  (chosen; drives linker FLASH)")
        else:
            print("  code-partition: (none)  - app uses whole flash from 0x08000000")

        # fixed-partitions children: label + reg
        parts: list[tuple[str, str, int, int]] = []
        for pm in re.finditer(
            r"([A-Za-z0-9_]+):\s*partition@[0-9a-fA-F]+\s*\{([^}]*)\}",
            dts_txt,
            re.S,
        ):
            node, body = pm.group(1), pm.group(2)
            lm = re.search(r'label\s*=\s*"([^"]+)"', body)
            rm = re.search(r"reg\s*=\s*<\s*(0x[0-9a-fA-F]+|\d+)\s+(0x[0-9a-fA-F]+|\d+)\s*>", body)
            if not lm or not rm:
                continue
            off = int(rm.group(1), 0)
            size = int(rm.group(2), 0)
            parts.append((node, lm.group(1), off, size))

        if parts:
            print()
            print("  DTS fixed-partitions (absolute Flash = 0x08000000 + offset):")
            print(f"  {'Node':<22} {'Label':<12} {'Offset':>10} {'Size':>10} {'Abs start':>12}  Role")
            for node, label, off, size in parts:
                abs_addr = 0x08000000 + off
                role = ""
                if code_part and node == code_part:
                    role = "<- THIS IMAGE"
                elif label == "code":
                    role = "standalone app"
                elif label == "mcuboot":
                    role = "bootloader"
                elif label == "image-0":
                    role = "primary slot"
                elif label == "image-1":
                    role = "upgrade slot"
                elif label == "storage":
                    role = "settings/NVS"
                mark = "*" if role.startswith("<-") else " "
                print(
                    f"  {mark}{node:<21} {label:<12} 0x{off:08X} {_fmt_size(size):>10} "
                    f"0x{abs_addr:08X}  {role}"
                )

    print()
    print("  Edit partitions :")
    print("    standalone : boards/.../<board>.dts  (single code_partition)")
    print("    MCUboot map: boards/.../support/partitions_mcuboot.dtsi")
    print("    App->slot0 : boards/.../<board>_mcuboot.overlay")
    print("    Boot link  : boards/.../support/mcuboot.overlay")
    print()
    return 0


def write_env_fingerprint(
    build_dir: Path,
    report: EnvReport,
    *,
    board: str,
    application: str,
) -> None:
    build_dir.mkdir(parents=True, exist_ok=True)
    zrev = None
    try:
        out = run(
            ["git", "rev-parse", "--short", "HEAD"],
            cwd=repo_root(),
            capture=True,
            check=False,
        )
        if out.returncode == 0:
            zrev = out.stdout.strip()
    except FileNotFoundError:
        pass

    txt = build_dir / "zplatform-environment.txt"
    txt.write_text(
        "\n".join(
            [
                "ZPlatform:",
                "  repository: ponmadasamy/zplatform",
                f"  revision: {zrev or 'unknown'}",
                "",
                "Zephyr:",
                f"  version: {report.zephyr.version or ''}",
                f"  revision: {report.zephyr.git_describe or report.cfg_revision}",
                "",
                "Zephyr SDK:",
                f"  version: {report.sdk.version or ''}",
                "",
                "Python:",
                f"  version: {report.python}",
                "",
                f"Board:",
                f"  {board}",
                "",
                f"Application:",
                f"  {application}",
                "",
            ]
        ),
        encoding="utf-8",
    )

    data = {
        "zplatform_revision": zrev or "unknown",
        "zephyr_version": report.zephyr.version or report.cfg_version,
        "zephyr_revision": report.zephyr.git_exact_tag
        or report.zephyr.git_describe
        or report.cfg_revision,
        "sdk_version": report.sdk.version or "",
        "python_version": report.python,
        "board": board,
        "application": application,
    }
    (build_dir / "zplatform-environment.json").write_text(
        json.dumps(data, indent=2) + "\n", encoding="utf-8"
    )


def export_env_file(path: Path, env: dict[str, str]) -> None:
    """Write key=value for shell sourcing (no secrets beyond paths)."""
    keys = [
        "ZEPHYR_BASE",
        "ZEPHYR_SDK_INSTALL_DIR",
        "ZEPHYR_TOOLCHAIN_VARIANT",
        "ZPLATFORM_ROOT",
        "ZPLATFORM_WEST_WORKSPACE",
        "VIRTUAL_ENV",
        "PATH",
    ]
    lines = [f"{k}={env[k]}" for k in keys if k in env]
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


# ---------------------------------------------------------------------------
# Latest check
# ---------------------------------------------------------------------------

def check_latest(cfg: dict[str, Any]) -> int:
    current = required_zephyr_revision(cfg)
    release_line = str(cfg["zephyr"]["release"])
    latest_stable = None
    latest_in_line = None
    try:
        import urllib.request

        url = "https://api.github.com/repos/zephyrproject-rtos/zephyr/releases?per_page=30"
        req = urllib.request.Request(url, headers={"User-Agent": "zplatform-env"})
        with urllib.request.urlopen(req, timeout=20) as resp:
            releases = json.loads(resp.read().decode("utf-8"))
        tags = [r.get("tag_name", "") for r in releases if not r.get("prerelease")]
        # Latest overall stable (first non-pre)
        if tags:
            latest_stable = tags[0]
        # Latest in release line
        line_tags = [t for t in tags if t.startswith(f"v{release_line}.") or t == f"v{release_line}"]
        if line_tags:
            latest_in_line = sorted(line_tags, key=parse_version, reverse=True)[0]
    except Exception as exc:
        warn(f"could not query Zephyr releases: {exc}")
        print(f"Current ZPlatform baseline:\n    {current}")
        print("Latest stable Zephyr:\n    (unavailable — offline or network error)")
        print("Status:\n    UNKNOWN")
        return 2

    print("Current ZPlatform baseline:")
    print(f"    {current}")
    print()
    print("Latest stable Zephyr:")
    print(f"    {latest_stable}")
    print()
    print(f"Latest in release line {release_line}:")
    print(f"    {latest_in_line or 'n/a'}")
    print()
    if latest_stable == current or latest_in_line == current:
        print("Status:\n    CURRENT")
        return 0
    print("Status:\n    UPDATE AVAILABLE")
    print()
    print("No automatic update performed.")
    print("To move the project baseline, update config/toolchain.yml and run:")
    print("    ./scripts/sh/setup-zephyr.sh --sync-manifest")
    print("    ./scripts/sh/setup-zephyr.sh --sync")
    return 0


# ---------------------------------------------------------------------------
# Ensure / sync
# ---------------------------------------------------------------------------

def cmd_ensure(cfg: dict[str, Any], *, sync: bool, install_sdk: bool) -> EnvReport:
    ensure_venv(cfg)
    ok, detail = validate_manifest_agreement(cfg)
    if not ok:
        die(
            f"Zephyr revision mismatch between config and west.yml ({detail}). "
            f"Run: ./scripts/sh/setup-zephyr.sh --sync-manifest"
        )
    ws = ensure_workspace(cfg, sync=sync)
    zephyr = inspect_zephyr(ws, cfg)
    if not zephyr.ok:
        info("Zephyr mismatch detected — running west update to pinned revision")
        run([str(venv_west()), "update"], cwd=ws, check=True)
        zephyr = inspect_zephyr(ws, cfg)
        if not zephyr.ok:
            die(zephyr.detail or "Zephyr still mismatched after west update")

    sdk = select_sdk(cfg)
    if not sdk.ok and install_sdk:
        sdk = try_install_sdk(ws, cfg)
    if not sdk.ok:
        die(sdk.detail or "SDK incompatible")

    report = gather_report(cfg, require_zephyr=True)
    return report


def cmd_print_env(cfg: dict[str, Any]) -> int:
    """Print shell-exportable environment after ensure (for wrappers)."""
    report = gather_report(cfg, require_zephyr=True)
    if not report.ready:
        print_status(report, cfg)
        die("environment not ready; run ./scripts/sh/setup-zephyr.sh --sync")
    assert report.workspace
    ws = Path(report.workspace)
    env = build_scoped_env(ws, report.sdk)
    # Machine-readable for wrappers
    out = {
        "ZEPHYR_BASE": env["ZEPHYR_BASE"],
        "ZEPHYR_SDK_INSTALL_DIR": env.get("ZEPHYR_SDK_INSTALL_DIR", ""),
        "ZEPHYR_TOOLCHAIN_VARIANT": env["ZEPHYR_TOOLCHAIN_VARIANT"],
        "ZPLATFORM_ROOT": env["ZPLATFORM_ROOT"],
        "ZPLATFORM_WEST_WORKSPACE": env["ZPLATFORM_WEST_WORKSPACE"],
        "ZPLATFORM_PYTHON": str(venv_python()),
        "ZPLATFORM_WEST": str(venv_west()),
        "VIRTUAL_ENV": env["VIRTUAL_ENV"],
        "PATH_PREFIX": str(venv_python().parent),
        "zephyr_version": report.zephyr.version,
        "zephyr_revision": report.zephyr.git_describe,
        "sdk_version": report.sdk.version,
        "python_version": report.python,
        "ready": report.ready,
    }
    print(json.dumps(out))
    return 0


def cmd_print_env_cmd(cfg: dict[str, Any]) -> int:
    """Print cmd.exe `set` lines for scripts/cmd (stdout only)."""
    report = gather_report(cfg, require_zephyr=True)
    if not report.ready:
        print_status(report, cfg)
        die("environment not ready; run scripts\\cmd\\setup-zephyr.cmd --sync")
    assert report.workspace
    ws = Path(report.workspace)
    env = build_scoped_env(ws, report.sdk)
    prefix = str(venv_python().parent)

    def _set(name: str, value: str) -> None:
        # Strip quotes; batch set "K=V" handles spaces.
        cleaned = str(value).replace('"', "")
        print(f'set "{name}={cleaned}"')

    _set("ZEPHYR_BASE", env["ZEPHYR_BASE"])
    _set("ZEPHYR_SDK_INSTALL_DIR", env.get("ZEPHYR_SDK_INSTALL_DIR", ""))
    _set("ZEPHYR_TOOLCHAIN_VARIANT", env["ZEPHYR_TOOLCHAIN_VARIANT"])
    _set("ZPLATFORM_ROOT", env["ZPLATFORM_ROOT"])
    _set("ZPLATFORM_WEST_WORKSPACE", env["ZPLATFORM_WEST_WORKSPACE"])
    _set("ZPLATFORM_PYTHON", str(venv_python()))
    _set("ZPLATFORM_WEST", str(venv_west()))
    _set("VIRTUAL_ENV", env["VIRTUAL_ENV"])
    # Literal %PATH% so cmd expands it when the set line runs.
    print(f'set "PATH={prefix};%PATH%"')
    return 0


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main(argv: Optional[list[str]] = None) -> int:
    parser = argparse.ArgumentParser(description="ZPlatform environment manager")
    parser.add_argument("--status", action="store_true")
    parser.add_argument("--ensure", action="store_true", help="create/validate env")
    parser.add_argument("--sync", action="store_true", help="west update to pin")
    parser.add_argument("--sync-manifest", action="store_true")
    parser.add_argument("--check-latest", action="store_true")
    parser.add_argument("--update", action="store_true", help="report only; no pin change")
    parser.add_argument("--print-env", action="store_true", help="JSON env for wrappers")
    parser.add_argument(
        "--print-env-cmd",
        action="store_true",
        help="cmd.exe set lines for scripts/cmd",
    )
    parser.add_argument("--install-sdk", action="store_true", default=True)
    parser.add_argument("--no-install-sdk", action="store_true")
    parser.add_argument("--write-fingerprint", metavar="BUILD_DIR")
    parser.add_argument(
        "--print-layout",
        metavar="BUILD_DIR",
        help="print flash partitions + linker LOAD_* from a finished build dir",
    )
    parser.add_argument("--board", default="")
    parser.add_argument("--application", default="")
    parser.add_argument("--config-name", default="default")
    parser.add_argument("--banner", action="store_true")
    parser.add_argument("--require-flash-tools", action="store_true")
    args = parser.parse_args(argv)

    cfg = load_toolchain()
    install_sdk = args.install_sdk and not args.no_install_sdk

    if args.sync_manifest:
        sync_west_manifest(cfg)
        return 0

    if args.check_latest or args.update:
        return check_latest(cfg)

    if args.print_layout:
        return print_flash_layout(Path(args.print_layout))

    if args.status and not args.ensure and not args.sync:
        # Light status may run before venv — try gather with what we have
        if not venv_python().is_file():
            print("Project venv not created yet. Run: ./scripts/sh/setup-zephyr.sh")
            print(f"Host Python: {host_python_version()} (need >= {required_python_min(cfg)})")
            return 1
        report = gather_report(cfg, require_zephyr=True)
        print_status(report, cfg)
        return 0 if report.ready else 1

    need_ensure = (
        args.ensure
        or args.sync
        or args.print_env
        or args.print_env_cmd
    )
    # Banner / fingerprint / flash-tool check can reuse an already-ensured env
    # (wrappers call Initialize / --ensure first). Avoid triple pip/ensure noise.
    need_report = (
        need_ensure
        or args.banner
        or args.write_fingerprint
        or args.require_flash_tools
        or args.status
    )
    if need_report:
        report: Optional[EnvReport] = None
        if need_ensure:
            # --ensure validates/creates; --sync alone forces west update.
            # (Mismatch still triggers west update inside cmd_ensure.)
            quiet_stdout = (
                (args.print_env or args.print_env_cmd)
                and not args.banner
                and not args.status
            )
            global _REDIRECT_CHILD_STDOUT
            if quiet_stdout:
                # Keep machine-readable output alone on stdout; send tool noise to stderr.
                old_out = sys.stdout
                sys.stdout = sys.stderr
                _REDIRECT_CHILD_STDOUT = True
                try:
                    report = cmd_ensure(cfg, sync=args.sync, install_sdk=install_sdk)
                finally:
                    _REDIRECT_CHILD_STDOUT = False
                    sys.stdout = old_out
            else:
                report = cmd_ensure(cfg, sync=args.sync, install_sdk=install_sdk)
        else:
            if not venv_python().is_file():
                die("Project venv missing — run ./scripts/sh/setup-zephyr.sh --ensure")
            report = gather_report(cfg, require_zephyr=True)
            if not report.ready and (args.banner or args.write_fingerprint):
                die("environment not ready; run ./scripts/sh/setup-zephyr.sh --sync")

        assert report is not None
        if args.banner:
            print_build_banner(
                report,
                board=args.board or default_board(cfg),
                application=args.application or "unknown",
                config=args.config_name,
            )
            if not report.ready:
                return 1
        if args.write_fingerprint:
            write_env_fingerprint(
                Path(args.write_fingerprint),
                report,
                board=args.board or default_board(cfg),
                application=args.application or "unknown",
            )
        if args.print_env_cmd:
            return cmd_print_env_cmd(cfg)
        if args.print_env:
            return cmd_print_env(cfg)
        if args.status:
            print_status(report, cfg)
        if args.require_flash_tools:
            flash = check_flash_tools()
            if not any(t.ok for t in flash.values()):
                die(
                    "No flash/debug runner tools found (OpenOCD / ST-Link / "
                    "CubeProgrammer / J-Link). Install one before flashing."
                )
        return 0 if report.ready else 1

    parser.print_help()
    return 2


if __name__ == "__main__":
    sys.exit(main())
