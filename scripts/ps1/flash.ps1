# Copyright (c) 2026 ZPlatform contributors
# SPDX-License-Identifier: Apache-2.0

[CmdletBinding()]
param(
    [Parameter(Position = 0)][string]$Kind,
    [Parameter(Position = 1)][string]$Name,
    [string]$Board,
    [switch]$Erase,
    [string]$Runner,
    [switch]$Help
)

$ErrorActionPreference = "Stop"
. "$PSScriptRoot\common.ps1"

if ($Help) {
    Write-Host "Usage: .\scripts\flash.ps1 [demo|sample <name>] [-Board BOARD] [-Erase] [-Runner NAME]"
    exit 0
}

Initialize-ZPlatformEnv
& (Get-ZPlatformVenvPython) $script:ZPlatformEnvPy --ensure --require-flash-tools
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

if ($Kind) {
    $app = Resolve-ZPlatformApp -Kind $Kind -Name $Name
    if (-not $Board) { $Board = $script:ZPlatformDefaultBoard }
    $buildDir = $null
    foreach ($cfg in @("default", "mcuboot", "debug", "release")) {
        $cand = Get-ZPlatformBuildDir -Board $Board -AppName $app.Name -Config $cfg
        if (Test-Path (Join-Path $cand "zephyr")) { $buildDir = $cand; break }
    }
    $appName = $app.Name
} else {
    $meta = Join-Path (Get-ZPlatformRoot) "build\.zplatform-last-build"
    if (-not (Test-Path $meta)) { throw "no previous build metadata" }
    $map = @{}
    Get-Content $meta | ForEach-Object {
        if ($_ -match "^(.*?)=(.*)$") { $map[$Matches[1]] = $Matches[2] }
    }
    $Board = $map["board"]
    $buildDir = $map["build_dir"]
    $appName = $map["application"]
}

$flashArgs = @("-d", $buildDir)
if ($Erase) { $flashArgs += "--erase" }
if ($Runner) { $flashArgs += @("-r", $Runner) }

Write-Host "Flashing board=$Board app=$appName dir=$buildDir"
Invoke-ZPlatformWest flash @flashArgs
Write-Host "Flash complete."
