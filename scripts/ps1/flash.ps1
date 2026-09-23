# Copyright (c) 2026 ZPlatform contributors
# SPDX-License-Identifier: Apache-2.0

[CmdletBinding()]
param(
    [Parameter(Position = 0)][string]$Kind,
    [Parameter(Position = 1)][string]$Name,
    [string]$Board,
    [string]$Config,
    [switch]$Erase,
    [string]$Runner,
    [switch]$Help
)

$ErrorActionPreference = "Stop"
. "$PSScriptRoot\common.ps1"

if ($Help) {
    Write-Host @"
Usage:
  .\scripts\ps1\flash.ps1 [demo|sample <name>] [-Board BOARD] [-Config CFG] [-Erase] [-Runner NAME]

  Never rebuilds - only programs an existing build/<board>/... tree.

  -Config default   Bare image (whole-flash link). Use after a plain build.ps1 demo.
  -Config mcuboot   Slot0 image only. Requires MCUboot already in boot_partition
                    (use mcuboot_demo.ps1 -Flash). Do not flash this at 0x08000000 alone.
  -Config profile|debug|release
                    Matching build.ps1 / profile.ps1 config name.
  -Erase            Full chip erase before program
  -Runner NAME      Override (stm32cubeprogrammer|openocd|jlink|...)

  No app args → last build from build/.zplatform-last-build

Examples:
  .\scripts\ps1\flash.ps1 demo -Config default
  .\scripts\ps1\flash.ps1 demo -Config mcuboot
  .\scripts\ps1\flash.ps1 sample gpio -Config default
"@
    exit 0
}

Initialize-ZPlatformEnv
& (Get-ZPlatformVenvPython) $script:ZPlatformEnvPy --require-flash-tools
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

if ($Kind) {
    if (-not $Board) { $Board = $script:ZPlatformDefaultBoard }
    $app = Resolve-ZPlatformApp -Kind $Kind -Name $Name -Board $Board
    $resolved = Resolve-ZPlatformBuildDir -Board $Board -Kind $app.Kind -Name $app.Name -Config $Config
    $buildDir = $resolved.BuildDir
    $appName = $app.Name
    $cfg = $resolved.Config
} else {
    $meta = Read-ZPlatformLastBuild
    if (-not $meta) { throw "no previous build metadata - build first or pass demo|sample" }
    if ($Config -and $Config -ne $meta.configuration) {
        if (-not $Board) { $Board = $meta.board }
        $resolved = Resolve-ZPlatformBuildDir -Board $Board -Kind $meta.kind -Name $meta.application -Config $Config
        $buildDir = $resolved.BuildDir
        $appName = $meta.application
        $cfg = $resolved.Config
        $Board = $resolved.Board
    } else {
        $Board = $meta.board
        $buildDir = $meta.build_dir
        $appName = $meta.application
        $cfg = $meta.configuration
        if (-not (Test-Path (Join-Path $buildDir "zephyr"))) {
            throw "last build missing at $buildDir - rebuild or pass demo|sample -Config"
        }
    }
}

$flashArgs = @("-d", $buildDir)
if ($Erase) { $flashArgs += "--erase" }
if ($Runner) { $flashArgs += @("-r", $Runner) }

Write-Host "Flashing board=$Board app=$appName config=$cfg dir=$buildDir"
Invoke-ZPlatformWest flash @flashArgs
Write-Host "Flash complete."
