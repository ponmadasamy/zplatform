# Copyright (c) 2026 ZPlatform contributors
# SPDX-License-Identifier: Apache-2.0
#
# Full MCUboot demo workflow: bootloader + signed app (+ optional upgrade).
#
#   .\scripts\ps1\mcuboot_demo.ps1 -Board stm3210e_eval -Build
#   .\scripts\ps1\mcuboot_demo.ps1 -Board stm3210e_eval -Build -Flash
#   .\scripts\ps1\mcuboot_demo.ps1 -Board stm3210e_eval -Build -Flash -Upgrade

[CmdletBinding()]
param(
    [string]$Board,
    [switch]$Build,
    [switch]$Flash,
    [switch]$Upgrade,
    [switch]$Clean,
    [switch]$CleanAll,
    [switch]$Help
)

$ErrorActionPreference = "Stop"
. "$PSScriptRoot\common.ps1"

if ($Help) {
    Write-Host @"
Usage:
  .\scripts\ps1\mcuboot_demo.ps1 [-Board BOARD] [-Build] [-Flash] [-Upgrade]
                                 [-Clean|-CleanAll]

Full MCUboot path for the per-board demo (demo/<board>/).

  -Board BOARD    Target (default: stm3210e_eval)
  -Build          Build bootloader + signed slot0 app (v1) + signed v2
                  (default if no -Flash/-Upgrade given)
  -Flash          Program MCUboot @ 0x08000000 and signed v1 @ slot0
  -Upgrade        After flash, program signed v2 @ slot1 and check console
  -Clean          Before build: remove demo/mcuboot + boot/mcuboot trees
  -CleanAll       Before build: remove entire build/<board>/

Standalone clean: .\scripts\ps1\clean.ps1 -Help
Standalone demo:  .\scripts\ps1\demo.ps1 -Help
"@
    exit 0
}

if (-not $Board) { $Board = $script:ZPlatformDefaultBoard }
if ($Clean -and $CleanAll) { throw "use only one of -Clean or -CleanAll" }
if ($Upgrade) { $Flash = $true }
if (-not $Build -and -not $Flash) {
    $Build = $true
}

$null = Resolve-ZPlatformApp -Kind demo -Name demo -Board $Board

if ($CleanAll) {
    [void](Remove-ZPlatformBuild -Board $Board)
} elseif ($Clean) {
    [void](Remove-ZPlatformBuild -Board $Board -Kind demo -Name demo -Config mcuboot)
    [void](Remove-ZPlatformBuild -Board $Board -Kind boot -Name mcuboot -Config default)
    # Workspace fallback path for cross-drive boot builds
    if ($env:ZPLATFORM_WEST_WORKSPACE) {
        $wsBoot = Join-Path $env:ZPLATFORM_WEST_WORKSPACE "build\zplatform\$Board\mcuboot"
        if (Test-Path -LiteralPath $wsBoot) {
            Write-Host "Cleaning $wsBoot"
            Remove-Item -LiteralPath $wsBoot -Recurse -Force
        }
    }
}

Initialize-ZPlatformEnv

$root = Get-ZPlatformRoot
$rootFwd = ($root -replace '\\', '/')
$key = Join-Path $root 'keys\root-ec-p256.pem'
if (-not (Test-Path $key)) {
    throw "Missing $key - see keys/README.md"
}

$ws = $env:ZPLATFORM_WEST_WORKSPACE
$west = Get-ZPlatformVenvWest
$py = Get-ZPlatformVenvPython
$imgtool = Join-Path $ws 'bootloader\mcuboot\scripts\imgtool.py'
$mcubootSrc = Join-Path $ws 'bootloader\mcuboot\boot\zephyr'
$mcubootConf = Join-Path $root "boards\st\$Board\support\mcuboot.conf"
$mcubootOv = Join-Path $root "boards\st\$Board\support\mcuboot.overlay"
if (-not (Test-Path -LiteralPath $mcubootConf)) {
    throw "no MCUboot support for board '$Board' (expected $mcubootConf)"
}

$mcubootDir = Get-ZPlatformBuildDir -Board $Board -Kind 'boot' -Name 'mcuboot' -Config 'default'
$appDir = Get-ZPlatformBuildDir -Board $Board -Kind 'demo' -Name 'demo' -Config 'mcuboot'
if ((Split-Path -Qualifier $mcubootDir) -ne (Split-Path -Qualifier $mcubootSrc)) {
    $mcubootDir = Join-Path $ws "build\zplatform\$Board\mcuboot"
}
$slotSize = '0x70000'
$header = '0x200'

function Invoke-ImgtoolSign {
    param([string]$InBin, [string]$OutBin, [string]$Version)
    & $py $imgtool sign `
        --key $key `
        --header-size $header `
        --align 8 `
        --version $Version `
        --slot-size $slotSize `
        --pad `
        $InBin $OutBin
    if ($LASTEXITCODE -ne 0) { throw "imgtool sign $Version failed" }
}

$bin = Join-Path $appDir 'zephyr\zephyr.bin'
$signedV1 = Join-Path $appDir 'zephyr\zephyr.signed.bin'
$signedV2 = Join-Path $appDir 'zephyr\zephyr.signed.v2.bin'
$bootBin = Join-Path $mcubootDir 'zephyr\zephyr.bin'

if ($Build) {
    Write-Host "=== 1) Build MCUboot bootloader (board=$Board) ==="
    New-Item -ItemType Directory -Force -Path $mcubootDir | Out-Null
    $confFwd = ($mcubootConf -replace '\\', '/')
    $ovFwd = ($mcubootOv -replace '\\', '/')
    Push-Location $ws
    try {
        & $west build -b $Board -d $mcubootDir $mcubootSrc -- `
            "-DBOARD_ROOT=$rootFwd" `
            "-DZEPHYR_EXTRA_MODULES=$rootFwd" `
            "-DZPLATFORM_ROOT=$rootFwd" `
            "-DCONF_FILE=$confFwd" `
            "-DDTC_OVERLAY_FILE=$ovFwd"
        if ($LASTEXITCODE -ne 0) { throw "MCUboot build failed: $LASTEXITCODE" }
    } finally {
        Pop-Location
    }

    Write-Host "=== 2) Build + sign application (slot0, v1.0.0) ==="
    & "$PSScriptRoot\build.ps1" demo -Board $Board -Mcuboot
    if ($LASTEXITCODE -ne 0) { throw "demo -Mcuboot build failed" }
    if (-not (Test-Path -LiteralPath $signedV1)) {
        throw "expected west-signed image missing: $signedV1"
    }

    Write-Host "=== 3) Sign v2.0.0 for slot1 upgrade demo ==="
    Invoke-ImgtoolSign -InBin $bin -OutBin $signedV2 -Version '2.0.0'
    Write-Host "SIGNED_V1=$signedV1  (from west build)"
    Write-Host "SIGNED_V2=$signedV2"
}

if (-not $Flash) {
    Write-Host "Build complete (board=$Board). Re-run with -Flash / -Upgrade for hardware."
    exit 0
}

if (-not (Test-Path -LiteralPath $bootBin) -or -not (Test-Path -LiteralPath $signedV1)) {
    throw "missing boot/app images - run with -Build first (boot=$bootBin app=$signedV1)"
}
if ($Upgrade -and -not (Test-Path -LiteralPath $signedV2)) {
    throw "missing v2 image for upgrade - run with -Build first ($signedV2)"
}

$cli = 'C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe'
if (-not (Test-Path $cli)) {
    throw "STM32_Programmer_CLI not found: $cli"
}

Write-Host "=== 4) Flash MCUboot + signed v1 (board=$Board) ==="
& $cli -c port=SWD mode=UR freq=4000 reset=HWrst `
    -e all `
    -w $bootBin 0x08000000 `
    -w $signedV1 0x08080000 `
    -v -rst -run 0x08000000
if ($LASTEXITCODE -ne 0) { throw "flash boot+v1 failed: $LASTEXITCODE" }

if (-not $Upgrade) {
    Write-Host 'Flashed boot + v1 (slot0 @ bank2 0x08080000). Use -Upgrade to exercise slot1 overwrite.'
    exit 0
}

Write-Host '=== 5) Flash signed v2 to slot1 (bank1) and verify upgrade ==='
Start-Sleep -Seconds 2
& $cli -c port=SWD mode=UR freq=4000 reset=HWrst `
    -w $signedV2 0x08010000 `
    -v -rst -run 0x08000000
if ($LASTEXITCODE -ne 0) { throw "flash v2 slot1 failed: $LASTEXITCODE" }

function Open-ZPlatformSerial {
    param([string]$Port = 'COM3', [int]$Tries = 20)
    for ($i = 1; $i -le $Tries; $i++) {
        try {
            $s = [System.IO.Ports.SerialPort]::new($Port, 115200)
            $s.ReadTimeout = 200
            $s.DtrEnable = $true
            $s.Open()
            return $s
        } catch {
            Start-Sleep -Milliseconds 500
        }
    }
    return $null
}

function Read-ZPlatformSerialBurst {
    param($Serial, [int]$Seconds = 15)
    $buf = ''
    $sw = [Diagnostics.Stopwatch]::StartNew()
    while ($sw.Elapsed.TotalSeconds -lt $Seconds) {
        Start-Sleep -Milliseconds 300
        try {
            if (-not $Serial.IsOpen) { break }
            $chunk = $Serial.ReadExisting()
            if ($chunk) { $buf += $chunk }
        } catch { break }
        if ($buf -match 'Image version:' -and $buf -match 'ZPlatform') { break }
    }
    return $buf
}

# CubeProgrammer / USB can briefly steal the ST-Link VCP - open after flash.
Start-Sleep -Seconds 2
$ser = Open-ZPlatformSerial
if (-not $ser) {
    Write-Host 'COM3 busy - close any serial monitor on COM3, then re-run with -Upgrade'
    Write-Host 'Manual check: Image version: v2.0.0 after swap-using-move completes'
    exit 1
}

$text = ''
try {
    $ser.DiscardInBuffer()
    & $cli -c port=SWD mode=UR freq=4000 reset=HWrst -rst -run 0x08000000 | Out-Null
    Write-Host 'Capturing upgrade start (30s)...'
    $text = Read-ZPlatformSerialBurst -Serial $ser -Seconds 30
    Write-Host $text
} finally {
    try { if ($ser -and $ser.IsOpen) { $ser.Close() } } catch { }
    try { if ($ser) { $ser.Dispose() } } catch { }
}

# Overwrite upgrade (bank2 primary): copy is usually <30s; do not reset mid-copy.
if ($text -notmatch 'Image version: v2\.0\.0') {
    $waitSec = 60
    if ($text -match 'copying the secondary slot' -or $text -match 'Erasing the primary') {
        Write-Host "Upgrade copy in progress - waiting ${waitSec}s without reset..."
    } else {
        Write-Host "Waiting ${waitSec}s for upgrade, then checking COM3..."
    }
    Start-Sleep -Seconds $waitSec

    $ser = Open-ZPlatformSerial -Tries 20
    if (-not $ser) {
        Write-Host 'COM3 busy after upgrade wait - close serial monitors and check manually for v2.0.0'
        exit 1
    }
    try {
        $ser.DiscardInBuffer()
        & $cli -c port=SWD mode=UR freq=4000 reset=HWrst -rst -run 0x08000000 | Out-Null
        $burst = Read-ZPlatformSerialBurst -Serial $ser -Seconds 25
        if ($burst) {
            $text += "`n--- post-upgrade ---`n" + $burst
            Write-Host $burst
        }
    } finally {
        try { if ($ser.IsOpen) { $ser.Close() } } catch { }
        try { $ser.Dispose() } catch { }
    }
}

Write-Host '--- VCP ---'
Write-Host $text
if ($text -match 'Image version: v2\.0\.0') {
    Write-Host 'UPGRADE_CHECK: PASS (Image version: v2.0.0)'
} else {
    Write-Host 'UPGRADE_CHECK: FAIL - expect Image version: v2.0.0 after swap completes'
    exit 1
}

Write-Host "mcuboot_demo.ps1 complete (board=$Board)."
