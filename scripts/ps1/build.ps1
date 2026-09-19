# Copyright (c) 2026 ZPlatform contributors
# SPDX-License-Identifier: Apache-2.0

param(
    [Parameter(Position = 0)][string]$Kind,
    [Parameter(Position = 1)][string]$Name,
    [string]$Board,
    [switch]$Clean,
    # Named DebugBuild (not Debug) to avoid PowerShell common-parameter clash.
    [switch]$DebugBuild,
    [switch]$Release,
    [switch]$Mcuboot,
    [switch]$Help
)

$ErrorActionPreference = "Stop"
. "$PSScriptRoot\common.ps1"

if ($Help -or -not $Kind) {
    Write-Host @"
Usage:
  .\scripts\build.ps1 demo [-Board BOARD] [-Clean] [-DebugBuild|-Release] [-Mcuboot]
  .\scripts\build.ps1 sample board_bringup [-Board BOARD] ...
"@
    exit $(if ($Help) { 0 } else { 1 })
}

if (-not $Board) { $Board = $script:ZPlatformDefaultBoard }
$Cfg = "default"
$Extra = @()
if ($DebugBuild) { $Cfg = "debug"; $Extra = @("-DCONFIG_DEBUG=y", "-DCONFIG_DEBUG_OPTIMIZATIONS=y", "-DCONFIG_THREAD_NAME=y") }
if ($Release) { $Cfg = "release"; $Extra = @("-DCONFIG_SIZE_OPTIMIZATIONS=y") }
if ($Mcuboot) { $Cfg = "mcuboot" }

Initialize-ZPlatformEnv
$app = Resolve-ZPlatformApp -Kind $Kind -Name $Name
$buildDir = Get-ZPlatformBuildDir -Board $Board -AppName $app.Name -Config $Cfg

& (Get-ZPlatformVenvPython) $script:ZPlatformEnvPy `
    --ensure --banner --board $Board --application $app.Name --config-name $Cfg
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

if ($Clean -and (Test-Path $buildDir)) { Remove-Item -Recurse -Force $buildDir }
New-Item -ItemType Directory -Force -Path $buildDir | Out-Null

# CMake/DTS path args must use forward slashes on Windows (backslash
# escapes break DTC_OVERLAY_FILE / BOARD_ROOT parsing under newer CMake).
$rootFwd = ($script:ZPlatformRoot -replace '\\', '/')
$buildFwd = ($buildDir -replace '\\', '/')
$appFwd = ($app.Path -replace '\\', '/')

$cmakeArgs = @(
    "-b", $Board, "-d", $buildFwd, $appFwd, "--",
    "-DBOARD_ROOT=$rootFwd",
    "-DZEPHYR_EXTRA_MODULES=$rootFwd"
) + $Extra

if ($Mcuboot) {
    $overlay = "$rootFwd/boards/st/stm3210e_eval/stm3210e_eval_mcuboot.overlay"
    $cmakeArgs += "-DCONFIG_BOOTLOADER_MCUBOOT=y"
    $cmakeArgs += "-DDTC_OVERLAY_FILE=$overlay"
}

Write-Host "Executing: west build $($cmakeArgs -join ' ')"
Invoke-ZPlatformWest build @cmakeArgs
Write-ZPlatformLastBuild -Board $Board -Application $app.Name -BuildDir $buildDir -Configuration $Cfg -Kind $app.Kind
& (Get-ZPlatformVenvPython) $script:ZPlatformEnvPy `
    --ensure --write-fingerprint $buildDir --board $Board --application $app.Name

Write-Host "Build complete: $buildDir"
