# Copyright (c) 2026 ZPlatform contributors
# SPDX-License-Identifier: Apache-2.0

[CmdletBinding()]
param(
    [Parameter(Position = 0)][string]$Focus,
    [string]$Board,
    [switch]$Native,
    [switch]$Help
)

$ErrorActionPreference = "Stop"
. "$PSScriptRoot\common.ps1"

if ($Help) {
    Write-Host "Usage: .\scripts\test.ps1 [platform/smoke] [-Board BOARD] [-Native]"
    exit 0
}

if (-not $Board) { $Board = $script:ZPlatformDefaultBoard }
Initialize-ZPlatformEnv

$root = Get-ZPlatformRoot
$testRoot = Join-Path $root "tests"
if ($Focus) { $testRoot = Join-Path $testRoot $Focus }
if (-not (Test-Path $testRoot)) { throw "test path not found: $testRoot" }

# Twister uses os.path.relpath against ZEPHYR_BASE; that fails when the
# repo and west workspace are on different Windows drives (E: vs C:).
$ws = $env:ZPLATFORM_WEST_WORKSPACE
if ($ws -and (Split-Path -Qualifier $root) -ne (Split-Path -Qualifier $ws)) {
    Write-Host @"
error: twister cannot load tests across Windows drives.
  repo      : $root
  workspace : $ws
Clone/link zplatform onto the same drive as the west workspace, or set
ZPLATFORM_WEST_WORKSPACE to a workspace on $((Split-Path -Qualifier $root)).
"@
    exit 1
}

$outdir = Join-Path $root "build\twister"
New-Item -ItemType Directory -Force -Path $outdir | Out-Null

Write-Host "========================================"
Write-Host "ZPlatform Twister"
Write-Host "========================================"
Write-Host "ZEPHYR_BASE : $($env:ZEPHYR_BASE)"
Write-Host "Workspace   : $($env:ZPLATFORM_WEST_WORKSPACE)"
Write-Host "west        : $($env:ZPLATFORM_WEST)"
Write-Host "========================================"

$argsList = @(
    "-T", $testRoot, "-O", $outdir, "-v",
    "--board-root", $root,
    "-x=-DZEPHYR_EXTRA_MODULES=$root",
    "-x=-DBOARD_ROOT=$root"
)
if ($Native) {
    $argsList += @("-p", "native_sim")
} else {
    $argsList += @("-p", $Board, "--build-only")
    Write-Host "Note: default mode is --build-only (no hardware execution)."
}

Invoke-ZPlatformWest twister @argsList
