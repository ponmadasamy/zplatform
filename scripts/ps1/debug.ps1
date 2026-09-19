# Copyright (c) 2026 ZPlatform contributors
# SPDX-License-Identifier: Apache-2.0

[CmdletBinding()]
param(
    [string]$Board,
    [string]$Runner,
    [switch]$Help
)

$ErrorActionPreference = "Stop"
. "$PSScriptRoot\common.ps1"

if ($Help) {
    Write-Host "Usage: .\scripts\debug.ps1 [-Board BOARD] [-Runner NAME]"
    exit 0
}

Initialize-ZPlatformEnv
$meta = Join-Path (Get-ZPlatformRoot) "build\.zplatform-last-build"
if (-not (Test-Path $meta)) { throw "no previous build - build first" }
$map = @{}
Get-Content $meta | ForEach-Object {
    if ($_ -match "^(.*?)=(.*)$") { $map[$Matches[1]] = $Matches[2] }
}
$buildDir = $map["build_dir"]
$argsList = @("-d", $buildDir)
if ($Runner) { $argsList += @("-r", $Runner) }
Write-Host "Debugging $buildDir\zephyr\zephyr.elf"
Invoke-ZPlatformWest debug @argsList
