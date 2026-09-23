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
  .\scripts\ps1\build.ps1 <demo|sample> [name] [options]

Kinds:
  demo              Per-board app: demo/<Board>/  (e.g. demo/stm3210e_eval)
  sample <name>     Shared sample: samples/<name>/  (board_bringup|gpio|sensor|spi_flash|adc|pwm|lcd|ext_sram|microsd|lvgl)

Options:
  -Board BOARD      Target board (default: stm3210e_eval)
  -Clean            Delete this config's build dir before configuring
  -DebugBuild       Config=debug   (assertions, thread names)
  -Release          Config=release (size opts)
  -Mcuboot          Config=mcuboot - link into slot0 and sign with imgtool
                    (zephyr.signed.bin). Requires keys/root-ec-p256.pem.

Standalone clean (targeted / board / all):
  .\scripts\ps1\clean.ps1 -Help

Configs write to separate trees (no overwrite):
  build/<board>/demo/<config>/
  build/<board>/sample/<name>/<config>/

default vs -Mcuboot (linker / partitions):
  default   Base DTS: single code_partition (whole 1 MiB) + zephyr,code-partition.
            Layout print marks code as <- THIS IMAGE.
  -Mcuboot  Overlays replace partitions with boot/slot0/slot1/storage and link
            the app into slot0 (448 KiB @ 0x80000 / bank2, header 0x200).

Where to change the flash map:
  boards/st/stm3210e_eval/stm3210e_eval.dts                 standalone code_partition
  boards/st/stm3210e_eval/support/partitions_mcuboot.dtsi   MCUboot 4-way map
  boards/st/stm3210e_eval/stm3210e_eval_mcuboot.overlay     app -> slot0
  boards/st/stm3210e_eval/support/mcuboot.overlay           bootloader -> boot
  boards/st/stm3210e_eval/support/mcuboot.conf              MCUboot Kconfig

After each build, a partition/linker summary is printed from the build tree.
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
$app = Resolve-ZPlatformApp -Kind $Kind -Name $Name -Board $Board
$buildDir = Get-ZPlatformBuildDir -Board $Board -Kind $app.Kind -Name $app.Name -Config $Cfg

& (Get-ZPlatformVenvPython) $script:ZPlatformEnvPy `
    --banner --board $Board --application $app.Name --config-name $Cfg
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

if ($Clean -and (Test-Path $buildDir)) {
    [void](Remove-ZPlatformBuild -Board $Board -Kind $app.Kind -Name $app.Name -Config $Cfg)
}
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
    $overlayPath = Join-Path $script:ZPlatformRoot "boards\st\$Board\${Board}_mcuboot.overlay"
    if (-not (Test-Path -LiteralPath $overlayPath)) {
        throw "MCUboot overlay not found for board $Board (expected $overlayPath)"
    }
    $keyPath = Join-Path $script:ZPlatformRoot "keys\root-ec-p256.pem"
    if (-not (Test-Path -LiteralPath $keyPath)) {
        throw "Missing signing key $keyPath - see keys/README.md"
    }
    $overlay = ($overlayPath -replace '\\', '/')
    $extraConf = Join-Path $app.Path "prj_mcuboot.conf"
    $cmakeArgs += "-DZPLATFORM_ROOT=$rootFwd"
    $cmakeArgs += "-DCONFIG_BOOTLOADER_MCUBOOT=y"
    $cmakeArgs += "-DCONFIG_STREAM_FLASH=y"
    $cmakeArgs += "-DCONFIG_IMG_MANAGER=y"
    $cmakeArgs += "-DCONFIG_MCUBOOT_IMG_MANAGER=y"
    $cmakeArgs += "-DDTC_OVERLAY_FILE=$overlay"
    if (Test-Path -LiteralPath $extraConf) {
        $cmakeArgs += "-DEXTRA_CONF_FILE=$(($extraConf -replace '\\', '/'))"
    }
}

Write-Host "Executing: west build $($cmakeArgs -join ' ')"
Invoke-ZPlatformWest build @cmakeArgs
Write-ZPlatformLastBuild -Board $Board -Application $app.Name -BuildDir $buildDir -Configuration $Cfg -Kind $app.Kind
& (Get-ZPlatformVenvPython) $script:ZPlatformEnvPy `
    --write-fingerprint $buildDir --board $Board --application $app.Name
& (Get-ZPlatformVenvPython) $script:ZPlatformEnvPy --print-layout $buildDir

Write-Host "Build complete: $buildDir"
