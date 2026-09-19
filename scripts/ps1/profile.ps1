# Copyright (c) 2026 ZPlatform contributors
# SPDX-License-Identifier: Apache-2.0

[CmdletBinding()]
param(
    [Parameter(Position = 0)][string]$Kind = "demo",
    [Parameter(Position = 1)][string]$Name,
    [string]$Board,
    [switch]$Help
)

$ErrorActionPreference = "Stop"
. "$PSScriptRoot\common.ps1"

if ($Help) {
    Write-Host "Usage: .\scripts\profile.ps1 [demo|sample <name>] [-Board BOARD]"
    exit 0
}

if (-not $Board) { $Board = $script:ZPlatformDefaultBoard }
Initialize-ZPlatformEnv
$app = Resolve-ZPlatformApp -Kind $Kind -Name $Name
$buildDir = Get-ZPlatformBuildDir -Board $Board -AppName $app.Name -Config "profile"
New-Item -ItemType Directory -Force -Path $buildDir | Out-Null

$cmakeArgs = @(
    "-b", $Board, "-d", $buildDir, $app.Path, "--",
    "-DBOARD_ROOT=$($script:ZPlatformRoot)",
    "-DZEPHYR_EXTRA_MODULES=$($script:ZPlatformRoot)",
    "-DCONFIG_ZPLATFORM_PROFILING=y",
    "-DCONFIG_THREAD_RUNTIME_STATS=y",
    "-DCONFIG_INIT_STACKS=y",
    "-DCONFIG_THREAD_STACK_INFO=y",
    "-DCONFIG_THREAD_MONITOR=y"
)
Invoke-ZPlatformWest build @cmakeArgs
Write-ZPlatformLastBuild -Board $Board -Application $app.Name -BuildDir $buildDir -Configuration "profile" -Kind $app.Kind
Write-Host "Profiling build ready: $buildDir"
