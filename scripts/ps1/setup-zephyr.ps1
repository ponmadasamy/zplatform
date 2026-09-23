# Copyright (c) 2026 ZPlatform contributors
# SPDX-License-Identifier: Apache-2.0
#
# ZPlatform Zephyr environment manager (Windows PowerShell).

[CmdletBinding()]
param(
    [switch]$Status,
    [switch]$Ensure,
    [switch]$Sync,
    [switch]$SyncManifest,
    [switch]$CheckLatest,
    [switch]$Update,
    [switch]$Help
)

$ErrorActionPreference = "Stop"
$Root = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$EnvPy = Join-Path $Root "scripts\env\zplatform_env.py"
$VenvPy = Join-Path $Root ".venv\Scripts\python.exe"

function Find-Python312 {
    $candidates = @(
        @{ Cmd = "py"; Args = @("-3.12") },
        @{ Cmd = "py"; Args = @("-3.13") },
        @{ Cmd = "py"; Args = @("-3") },
        @{ Cmd = "python"; Args = @() },
        @{ Cmd = "python3"; Args = @() }
    )
    foreach ($c in $candidates) {
        $cmd = Get-Command $c.Cmd -ErrorAction SilentlyContinue
        if (-not $cmd) { continue }
        try {
            $ver = & $c.Cmd @($c.Args) -c "import sys; print('%d.%d'%sys.version_info[:2])" 2>$null
            if ($ver -match '^(\d+)\.(\d+)$') {
                $major = [int]$Matches[1]; $minor = [int]$Matches[2]
                if ($major -gt 3 -or ($major -eq 3 -and $minor -ge 12)) {
                    return @{ Exe = $c.Cmd; Args = $c.Args }
                }
            }
        } catch { }
    }
    return $null
}

function Ensure-Venv {
    if (Test-Path $VenvPy) { return }
    $py = Find-Python312
    if (-not $py) {
        Write-Error "Python >= 3.12 required. Install from python.org or use 'py -3.12'."
    }
    Write-Host "Bootstrapping .venv with $($py.Exe) $($py.Args -join ' ')"
    & $py.Exe @($py.Args) -m venv (Join-Path $Root ".venv")
    # Pip/requirements: single path via zplatform_env.py --ensure (hash-stamped).
}

if ($Help) {
    Write-Host @"
Usage: .\scripts\setup-zephyr.ps1 [options]

  -Status           Show environment status
  -Ensure           Create/validate environment
  -Sync             west update to pinned Zephyr revision
  -SyncManifest     Copy toolchain.yml revision into west.yml
  -CheckLatest      Query official Zephyr releases (no pin change)
  -Update           Same as -CheckLatest
"@
    exit 0
}

Ensure-Venv

$pyArgs = @($EnvPy)
if ($Status) { $pyArgs += "--status" }
if ($Ensure) { $pyArgs += "--ensure" }
if ($Sync) { $pyArgs += "--sync" }
if ($SyncManifest) { $pyArgs += "--sync-manifest" }
if ($CheckLatest) { $pyArgs += "--check-latest" }
if ($Update) { $pyArgs += "--update" }
if (-not ($Status -or $Ensure -or $Sync -or $SyncManifest -or $CheckLatest -or $Update)) {
    $pyArgs += @("--ensure", "--status")
}

& $VenvPy @pyArgs
exit $LASTEXITCODE
