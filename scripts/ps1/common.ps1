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
    # Nested scripts (e.g. mcuboot_demo → build) inherit this and skip a
    # second full --ensure / requirements check.
    if ($env:ZPLATFORM_ENV_READY -eq "1" -and $env:ZEPHYR_BASE -and (Test-Path $env:ZEPHYR_BASE)) {
        return
    }
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
    $env:ZPLATFORM_ENV_READY = "1"

    if (-not (Test-Path $env:ZEPHYR_BASE)) {
        throw "ZEPHYR_BASE not established by environment manager"
    }
}

function Resolve-ZPlatformApp {
    param(
        [string]$Kind,
        [string]$Name,
        [string]$Board
    )
    $root = Get-ZPlatformRoot
    switch ($Kind) {
        "demo" {
            if (-not $Board) { $Board = $script:ZPlatformDefaultBoard }
            $path = Join-Path $root "demo\$Board"
            if (-not (Test-Path -LiteralPath $path)) {
                throw "no demo for board '$Board' (expected $path)"
            }
            return @{ Kind = "demo"; Name = "demo"; Path = $path; Board = $Board }
        }
        "sample" {
            if (-not $Name) { throw "sample name required" }
            return @{ Kind = "sample"; Name = $Name; Path = Join-Path $root "samples\$Name" }
        }
        "boot" {
            if (-not $Name) { $Name = "mcuboot" }
            return @{ Kind = "boot"; Name = $Name; Path = $null; Board = $Board }
        }
        default { throw "unknown kind $Kind" }
    }
}

# Layout: build/<board>/demo/<config>/
#         build/<board>/sample/<name>/<config>/
#         build/<board>/boot/<name>/<config>/
function Get-ZPlatformBuildDir {
    param(
        [string]$Board,
        [string]$Kind,
        [string]$Name,
        [string]$Config = "default"
    )
    if (-not $Config) { $Config = "default" }
    $root = Get-ZPlatformRoot
    switch ($Kind) {
        "demo" {
            return Join-Path $root "build\$Board\demo\$Config"
        }
        "sample" {
            if (-not $Name) { throw "sample name required for build dir" }
            return Join-Path $root "build\$Board\sample\$Name\$Config"
        }
        "boot" {
            if (-not $Name) { $Name = "mcuboot" }
            return Join-Path $root "build\$Board\boot\$Name\$Config"
        }
        default { throw "unknown kind '$Kind' for build dir" }
    }
}

function Get-ZPlatformAppParentDir {
    param([string]$Board, [string]$Kind, [string]$Name)
    $root = Get-ZPlatformRoot
    switch ($Kind) {
        "demo" { return Join-Path $root "build\$Board\demo" }
        "sample" { return Join-Path $root "build\$Board\sample\$Name" }
        "boot" {
            if (-not $Name) { $Name = "mcuboot" }
            return Join-Path $root "build\$Board\boot\$Name"
        }
        default { throw "unknown kind '$Kind'" }
    }
}

function Get-ZPlatformAvailableConfigs {
    param([string]$Board, [string]$Kind, [string]$Name)
    $parent = Get-ZPlatformAppParentDir -Board $Board -Kind $Kind -Name $Name
    if (-not (Test-Path -LiteralPath $parent)) { return @() }
    return @(Get-ChildItem -LiteralPath $parent -Directory |
        Where-Object { Test-Path (Join-Path $_.FullName "zephyr") } |
        ForEach-Object { $_.Name })
}

# Remove build artifacts. Returns list of removed paths.
# -All: entire build/
# -Board only: build/<board>/
# -Board -Kind [-Name] [-Config]: one tree or all configs under that app
function Remove-ZPlatformBuild {
    param(
        [switch]$All,
        [string]$Board,
        [string]$Kind,
        [string]$Name,
        [string]$Config
    )
    $root = Get-ZPlatformRoot
    $buildRoot = Join-Path $root "build"
    $removed = @()

    function Remove-One([string]$path) {
        if (Test-Path -LiteralPath $path) {
            Write-Host "Cleaning $path"
            Remove-Item -LiteralPath $path -Recurse -Force
            $script:__zp_removed += $path
        }
    }

    $script:__zp_removed = @()

    if ($All) {
        if (Test-Path -LiteralPath $buildRoot) {
            Write-Host "Cleaning all under $buildRoot"
            Get-ChildItem -LiteralPath $buildRoot -Force | ForEach-Object {
                Remove-Item -LiteralPath $_.FullName -Recurse -Force
                $script:__zp_removed += $_.FullName
            }
            # Keep build/ itself; drop last-build pointer if present
            $meta = Join-Path $buildRoot ".zplatform-last-build"
            if (Test-Path -LiteralPath $meta) {
                Remove-Item -LiteralPath $meta -Force
                $script:__zp_removed += $meta
            }
        } else {
            Write-Host "Nothing to clean ($buildRoot missing)"
        }
        return $script:__zp_removed
    }

    if (-not $Board) { throw "Board required unless -All" }

    if (-not $Kind) {
        Remove-One (Join-Path $buildRoot $Board)
        return $script:__zp_removed
    }

    if ($Config) {
        $dir = Get-ZPlatformBuildDir -Board $Board -Kind $Kind -Name $Name -Config $Config
        Remove-One $dir
        return $script:__zp_removed
    }

    # All configs for this app
    $parent = Get-ZPlatformAppParentDir -Board $Board -Kind $Kind -Name $Name
    Remove-One $parent
    return $script:__zp_removed
}

# Resolve an existing build tree for flash/debug. Never rebuilds.
# Priority: explicit Config > matching last-build > sole config > default if present.
function Resolve-ZPlatformBuildDir {
    param(
        [string]$Board,
        [string]$Kind,
        [string]$Name,
        [string]$Config
    )
    if (-not $Board) { $Board = $script:ZPlatformDefaultBoard }

    if ($Config) {
        $dir = Get-ZPlatformBuildDir -Board $Board -Kind $Kind -Name $Name -Config $Config
        if (-not (Test-Path (Join-Path $dir "zephyr"))) {
            $avail = Get-ZPlatformAvailableConfigs -Board $Board -Kind $Kind -Name $Name
            $hint = if ($avail.Count) { "available: $($avail -join ', ')" } else { "none built yet" }
            throw "no build at $dir ($hint)"
        }
        return @{ Board = $Board; Kind = $Kind; Name = $Name; Config = $Config; BuildDir = $dir }
    }

    $meta = Read-ZPlatformLastBuild
    if ($meta -and $meta.board -eq $Board -and $meta.kind -eq $Kind -and $meta.application -eq $Name) {
        if (Test-Path (Join-Path $meta.build_dir "zephyr")) {
            return @{
                Board = $Board; Kind = $Kind; Name = $Name
                Config = $meta.configuration; BuildDir = $meta.build_dir
            }
        }
    }

    $avail = Get-ZPlatformAvailableConfigs -Board $Board -Kind $Kind -Name $Name
    if ($avail.Count -eq 0) {
        throw "no builds for $Kind/$Name on $Board - run build first"
    }
    if ($avail.Count -eq 1) {
        $c = $avail[0]
        return @{
            Board = $Board; Kind = $Kind; Name = $Name; Config = $c
            BuildDir = (Get-ZPlatformBuildDir -Board $Board -Kind $Kind -Name $Name -Config $c)
        }
    }
    if ($avail -contains "default") {
        return @{
            Board = $Board; Kind = $Kind; Name = $Name; Config = "default"
            BuildDir = (Get-ZPlatformBuildDir -Board $Board -Kind $Kind -Name $Name -Config "default")
        }
    }
    throw "multiple configs for $Kind/$Name on $Board ($($avail -join ', ')) - pass -Config"
}

function Read-ZPlatformLastBuild {
    $meta = Join-Path (Get-ZPlatformRoot) "build\.zplatform-last-build"
    if (-not (Test-Path -LiteralPath $meta)) { return $null }
    $map = @{}
    Get-Content -LiteralPath $meta | ForEach-Object {
        if ($_ -match "^(.*?)=(.*)$") { $map[$Matches[1]] = $Matches[2] }
    }
    return [pscustomobject]@{
        board         = $map["board"]
        kind          = $map["kind"]
        application   = $map["application"]
        configuration = $map["configuration"]
        build_dir     = $map["build_dir"]
    }
}

function Write-ZPlatformLastBuild {
    param($Board, $Application, $BuildDir, $Configuration, $Kind)
    $metaDir = Join-Path (Get-ZPlatformRoot) "build"
    New-Item -ItemType Directory -Force -Path $metaDir | Out-Null
    @"
board=$Board
kind=$Kind
application=$Application
configuration=$Configuration
build_dir=$BuildDir
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
