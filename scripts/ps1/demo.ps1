# Copyright (c) 2026 ZPlatform contributors
# SPDX-License-Identifier: Apache-2.0
#
# High-level standalone demo workflow (no MCUboot).
#
#   .\scripts\ps1\demo.ps1 -Board stm3210e_eval
#   .\scripts\ps1\demo.ps1 -Board stm3210e_eval -Flash
#   .\scripts\ps1\demo.ps1 -Board stm3210e_eval -Build -Flash -Debug

[CmdletBinding()]
param(
    [string]$Board,
    [switch]$Build,
    [switch]$Flash,
    # Named DebugSession (not Debug) to avoid PowerShell common-parameter clash.
    [switch]$DebugSession,
    [switch]$Clean,
    [switch]$CleanAll,
    [switch]$DebugBuild,
    [switch]$Release,
    [switch]$Help
)

$ErrorActionPreference = "Stop"
. "$PSScriptRoot\common.ps1"

if ($Help) {
    Write-Host @"
Usage:
  .\scripts\ps1\demo.ps1 [-Board BOARD] [-Build] [-Flash] [-DebugSession]
                         [-Clean|-CleanAll] [-DebugBuild|-Release]

Standalone per-board demo (demo/<board>/), whole-flash link - not MCUboot.

  -Board BOARD      Target (default: stm3210e_eval). Must have demo/<board>/.
  -Build            Build demo (default if no -Flash/-DebugSession given)
  -Flash            Flash this demo config (builds first if tree missing)
  -DebugSession     Attach debugger to this demo ELF
  -Clean            Before build: remove this config's build tree only
  -CleanAll         Before build: remove entire build/<board>/
  -DebugBuild       Build config=debug
  -Release          Build config=release

Standalone clean (no build):
  .\scripts\ps1\clean.ps1 demo -Board BOARD -Config default
  .\scripts\ps1\clean.ps1 -Board BOARD
  .\scripts\ps1\clean.ps1 -All

Examples:
  .\scripts\ps1\demo.ps1 -Board stm3210e_eval -Clean
  .\scripts\ps1\demo.ps1 -Board stm3210e_eval -Build -Flash
  .\scripts\ps1\demo.ps1 -Flash

MCUboot / upgrade flow: .\scripts\ps1\mcuboot_demo.ps1 -Help
"@
    exit 0
}

if (-not $Board) { $Board = $script:ZPlatformDefaultBoard }
if ($Clean -and $CleanAll) { throw "use only one of -Clean or -CleanAll" }

# Default action: build
if (-not $Build -and -not $Flash -and -not $DebugSession) {
    $Build = $true
}

$cfg = "default"
if ($DebugBuild) { $cfg = "debug" }
if ($Release) { $cfg = "release" }
if ($DebugBuild -and $Release) {
    throw "use only one of -DebugBuild or -Release"
}

# Ensure demo exists for board
$null = Resolve-ZPlatformApp -Kind demo -Name demo -Board $Board

if ($CleanAll) {
    [void](Remove-ZPlatformBuild -Board $Board)
} elseif ($Clean -and -not $Build) {
    # Clean-only invocation
    [void](Remove-ZPlatformBuild -Board $Board -Kind demo -Name demo -Config $cfg)
    Write-Host "demo.ps1 cleaned board=$Board config=$cfg (no build)."
    exit 0
}

if ($Build) {
    Write-Host "=== demo: build (config=$cfg, board=$Board) ==="
    $bparams = @{ Board = $Board }
    if ($Clean -and -not $CleanAll) { $bparams['Clean'] = $true }
    if ($DebugBuild) { $bparams['DebugBuild'] = $true }
    if ($Release) { $bparams['Release'] = $true }
    & "$PSScriptRoot\build.ps1" demo @bparams
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

if ($Flash) {
    if (-not $Build) {
        $dir = Get-ZPlatformBuildDir -Board $Board -Kind demo -Name demo -Config $cfg
        if (-not (Test-Path (Join-Path $dir "zephyr"))) {
            Write-Host "=== demo: no $cfg tree - building first ==="
            $bparams = @{ Board = $Board }
            if ($DebugBuild) { $bparams['DebugBuild'] = $true }
            if ($Release) { $bparams['Release'] = $true }
            & "$PSScriptRoot\build.ps1" demo @bparams
            if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        }
    }
    Write-Host "=== demo: flash (config=$cfg, board=$Board) ==="
    & "$PSScriptRoot\flash.ps1" demo -Board $Board -Config $cfg
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

if ($DebugSession) {
    Write-Host "=== demo: debug (config=$cfg, board=$Board) ==="
    & "$PSScriptRoot\debug.ps1" demo -Board $Board -Config $cfg
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

Write-Host "demo.ps1 done (board=$Board config=$cfg)."
