# Copyright (c) 2026 ZPlatform contributors
# SPDX-License-Identifier: Apache-2.0
#
# Clean ZPlatform build outputs.
#
#   .\scripts\ps1\clean.ps1 -All
#   .\scripts\ps1\clean.ps1 -Board stm3210e_eval
#   .\scripts\ps1\clean.ps1 demo -Board stm3210e_eval -Config default
#   .\scripts\ps1\clean.ps1 sample gpio -Board stm3210e_eval

[CmdletBinding()]
param(
    [Parameter(Position = 0)][string]$Kind,
    [Parameter(Position = 1)][string]$Name,
    [string]$Board,
    [string]$Config,
    [switch]$All,
    [switch]$Help
)

$ErrorActionPreference = "Stop"
. "$PSScriptRoot\common.ps1"

if ($Help) {
    Write-Host @"
Usage:
  .\scripts\ps1\clean.ps1 -All
  .\scripts\ps1\clean.ps1 -Board BOARD
  .\scripts\ps1\clean.ps1 demo [-Board BOARD] [-Config CFG]
  .\scripts\ps1\clean.ps1 sample <name> [-Board BOARD] [-Config CFG]
  .\scripts\ps1\clean.ps1 boot [mcuboot] [-Board BOARD] [-Config CFG]

Scopes:
  -All                 Delete everything under build/ (all boards, twister, last-build)
  -Board BOARD         Delete build/<board>/ only
  kind [-Config CFG]   Targeted: one config, or all configs for that app if -Config omitted

Before-build clean (same as targeted on the tree about to build):
  .\scripts\ps1\build.ps1 demo -Board BOARD -Clean
  .\scripts\ps1\demo.ps1 -Board BOARD -Clean
  .\scripts\ps1\mcuboot_demo.ps1 -Board BOARD -Clean
"@
    exit 0
}

if ($All) {
    if ($Kind -or $Board -or $Config) {
        throw "-All cannot be combined with -Board / kind / -Config"
    }
    $n = Remove-ZPlatformBuild -All
    Write-Host "Cleaned $($n.Count) item(s) (all)."
    exit 0
}

if (-not $Board) { $Board = $script:ZPlatformDefaultBoard }

if (-not $Kind) {
    $n = Remove-ZPlatformBuild -Board $Board
    Write-Host "Cleaned board=$Board ($($n.Count) path(s))."
    exit 0
}

switch ($Kind) {
    "demo" {
        if (-not $Name) { $Name = "demo" }
    }
    "sample" {
        if (-not $Name) { throw "sample name required" }
    }
    "boot" {
        if (-not $Name) { $Name = "mcuboot" }
    }
    default { throw "unknown kind '$Kind' (demo|sample|boot)" }
}

$n = Remove-ZPlatformBuild -Board $Board -Kind $Kind -Name $Name -Config $Config
$scope = if ($Config) { "$Kind/$Name/$Config" } else { "$Kind/$Name (all configs)" }
Write-Host "Cleaned board=$Board $scope ($($n.Count) path(s))."
