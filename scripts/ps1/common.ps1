# Copyright (c) 2026 ZPlatform contributors
# SPDX-License-Identifier: Apache-2.0
#
# Shared PowerShell helpers - mirrors scripts/common.sh semantics.

$ErrorActionPreference = "Stop"

# scripts/ps1 -> repository root
$script:ZPlatformRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$script:ZPlatformDefaultBoard = "stm3210e_eval"
$script:ZPlatformEnvPy = Join-Path $script:ZPlatformRoot "scripts\env\zplatform_env.py"

function Get-ZPlatformRoot { return $script:ZPlatformRoot }

function Get-ZPlatformVenvPython {
    $p = Join-Path $script:ZPlatformRoot ".venv\Scripts\python.exe"
    if (-not (Test-Path $p)) { throw "Missing $p - run: .\scripts\ps1\setup-zephyr.ps1 first" }
    return $p
}

function Get-ZPlatformVenvWest {
    $p = Join-Path $script:ZPlatformRoot ".venv\Scripts\west.exe"
    if (-not (Test-Path $p)) { throw "Missing $p - run: .\scripts\ps1\setup-zephyr.ps1 first" }
    return $p
}

function Initialize-ZPlatformEnv {
    $setup = Join-Path $PSScriptRoot "setup-zephyr.ps1"
    $vpy = Join-Path $script:ZPlatformRoot ".venv\Scripts\python.exe"
    if (-not (Test-Path $vpy)) {
        & $setup -Ensure
    }
    $py = Get-ZPlatformVenvPython
    # stdout is JSON only; ensure chatter goes to stderr
    $json = & $py $script:ZPlatformEnvPy --ensure --print-env
    if ($LASTEXITCODE -ne 0) { throw "environment ensure failed ($LASTEXITCODE)" }
    $data = ($json | Out-String).Trim() | ConvertFrom-Json

    $env:ZEPHYR_BASE = $data.ZEPHYR_BASE
    $env:ZEPHYR_SDK_INSTALL_DIR = $data.ZEPHYR_SDK_INSTALL_DIR
    $env:ZEPHYR_TOOLCHAIN_VARIANT = $data.ZEPHYR_TOOLCHAIN_VARIANT
    $env:ZPLATFORM_ROOT = $data.ZPLATFORM_ROOT
    $env:ZPLATFORM_WEST_WORKSPACE = $data.ZPLATFORM_WEST_WORKSPACE
    $env:ZPLATFORM_PYTHON = $data.ZPLATFORM_PYTHON
    $env:ZPLATFORM_WEST = $data.ZPLATFORM_WEST
    $env:VIRTUAL_ENV = $data.VIRTUAL_ENV
    $env:PATH = "$($data.PATH_PREFIX);$($env:PATH)"

    if (-not (Test-Path $env:ZEPHYR_BASE)) {
        throw "ZEPHYR_BASE not established by environment manager"
    }
}

function Resolve-ZPlatformApp {
    param([string]$Kind, [string]$Name)
    $root = Get-ZPlatformRoot
    switch ($Kind) {
        "demo" {
            return @{ Kind = "demo"; Name = "demo"; Path = Join-Path $root "demo\zplatform_demo" }
        }
        "sample" {
            if (-not $Name) { throw "sample name required" }
            return @{ Kind = "sample"; Name = $Name; Path = Join-Path $root "samples\$Name" }
        }
        default { throw "unknown kind $Kind" }
    }
}

function Get-ZPlatformBuildDir {
    param([string]$Board, [string]$AppName, [string]$Config = "default")
    return Join-Path (Get-ZPlatformRoot) "build\$Board\$AppName\$Config"
}

function Write-ZPlatformLastBuild {
    param($Board, $Application, $BuildDir, $Configuration, $Kind)
    $metaDir = Join-Path (Get-ZPlatformRoot) "build"
    New-Item -ItemType Directory -Force -Path $metaDir | Out-Null
    @"
board=$Board
application=$Application
build_dir=$BuildDir
configuration=$Configuration
kind=$Kind
"@ | Set-Content -Path (Join-Path $metaDir ".zplatform-last-build") -Encoding utf8
}

function Invoke-ZPlatformWest {
    param([Parameter(ValueFromRemainingArguments = $true)]$WestArgs)
    # Run from the repo (same drive as app/build paths). Do NOT cd into the west
    # workspace: on Windows, west's os.path.relpath() fails when cwd is on C:
    # (e.g. ~/zephyrproject) and the source tree is on another drive (E:).
    # ZEPHYR_BASE (set by Initialize-ZPlatformEnv) lets west find the workspace.
    Push-Location $script:ZPlatformRoot
    try {
        & (Get-ZPlatformVenvWest) @WestArgs
        if ($LASTEXITCODE -ne 0) { throw "west failed with exit $LASTEXITCODE" }
    } finally {
        Pop-Location
    }
}

# Load default board from toolchain.yml if present
$tc = Join-Path $script:ZPlatformRoot "config\toolchain.yml"
if (Test-Path $tc) {
    $m = Select-String -Path $tc -Pattern 'default_board:\s*"?([^"\s]+)"?' | Select-Object -First 1
    if ($m) { $script:ZPlatformDefaultBoard = $m.Matches[0].Groups[1].Value }
}
