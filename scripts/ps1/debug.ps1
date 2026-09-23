# Copyright (c) 2026 ZPlatform contributors
# SPDX-License-Identifier: Apache-2.0

[CmdletBinding()]
param(
    [Parameter(Position = 0)][string]$Kind,
    [Parameter(Position = 1)][string]$Name,
    [string]$Board,
    [string]$Config,
    [string]$Runner,
    [switch]$Batch,
    [switch]$Help
)

$ErrorActionPreference = "Stop"
. "$PSScriptRoot\common.ps1"

if ($Help) {
    Write-Host @"
Usage:
  .\scripts\ps1\debug.ps1 [demo|sample <name>] [-Board BOARD] [-Config CFG] [-Runner NAME]

  Never rebuilds - attaches GDB to an existing ELF.
  Pick the same -Config you built (default vs mcuboot use different load addresses).

  Default runner is OpenOCD. On Windows this script starts OpenOCD, waits until
  the GDB port is listening, then launches GDB (avoids a west race that drops
  the remote link).

  STM3210E-EVAL + STLINK-V3MINI:
  - If Tamper put the MCU in STANDBY, press Wakeup (or flash again) before debug.
  - GDB attaches halted; type 'c' to continue, 'monitor reset halt' to reset.
  - Quit with 'quit'.

  -Batch   Non-interactive smoke: print pc/bt and quit (for CI / script checks).

Examples:
  .\scripts\ps1\debug.ps1 demo -Config default
  .\scripts\ps1\debug.ps1 sample gpio
  .\scripts\ps1\debug.ps1 sample gpio -Batch
"@
    exit 0
}

function Wait-TcpPort {
    param([string]$HostName = "127.0.0.1", [int]$Port = 3333, [int]$TimeoutSec = 20)
    $deadline = (Get-Date).AddSeconds($TimeoutSec)
    while ((Get-Date) -lt $deadline) {
        try {
            $client = [System.Net.Sockets.TcpClient]::new()
            $iar = $client.BeginConnect($HostName, $Port, $null, $null)
            if ($iar.AsyncWaitHandle.WaitOne(500)) {
                $client.EndConnect($iar)
                $client.Close()
                return $true
            }
            $client.Close()
        } catch {
            # retry
        }
        Start-Sleep -Milliseconds 250
    }
    return $false
}

function Get-RunnerTools {
    param([string]$BuildDir)
    $yaml = Join-Path $BuildDir "zephyr\runners.yaml"
    if (-not (Test-Path $yaml)) {
        throw "runners.yaml missing at $yaml - build the app first"
    }
    $text = Get-Content $yaml -Raw
    $gdb = [regex]::Match($text, '(?m)^\s*gdb:\s*(.+)\s*$').Groups[1].Value.Trim()
    $openocd = [regex]::Match($text, '(?m)^\s*openocd:\s*(.+)\s*$').Groups[1].Value.Trim()
    $cfg = [regex]::Match($text, '--config=([^\s]+)').Groups[1].Value.Trim()
    $search = @()
    foreach ($m in [regex]::Matches($text, '(?m)^\s*-\s*(C:/[^\r\n]+openocd/scripts)\s*$')) {
        $search += $m.Groups[1].Value.Trim()
    }
    if (-not $gdb -or -not (Test-Path $gdb)) { throw "gdb path not found in runners.yaml: $gdb" }
    if (-not $openocd -or -not (Test-Path $openocd)) { throw "openocd path not found in runners.yaml: $openocd" }
    if (-not $cfg -or -not (Test-Path $cfg)) { throw "openocd --config not found: $cfg" }
    return [pscustomobject]@{
        Gdb     = $gdb
        OpenOcd = $openocd
        Config  = $cfg
        Search  = $search
        Elf     = (Join-Path $BuildDir "zephyr\zephyr.elf")
    }
}

function Start-OpenOcdDebugServer {
    param($Tools)
    Get-Process -Name openocd -ErrorAction SilentlyContinue | Stop-Process -Force
    Start-Sleep -Milliseconds 300

    function Escape-WinArg([string]$s) {
        if ($s -notmatch '[ \t"]') { return $s }
        return '"' + ($s -replace '(\\*)"', '$1$1\"' -replace '(\\+)$', '$1$1') + '"'
    }

    $tokens = [System.Collections.Generic.List[string]]::new()
    $tokens.Add("-f"); $tokens.Add($Tools.Config)
    foreach ($s in $Tools.Search) {
        $tokens.Add("-s"); $tokens.Add($s)
    }
    foreach ($c in @(
            "tcl_port 6666",
            "telnet_port 4444",
            "gdb_port 3333",
            "init",
            "targets",
            "reset halt"
        )) {
        $tokens.Add("-c"); $tokens.Add($c)
    }
    $argLine = ($tokens | ForEach-Object { Escape-WinArg $_ }) -join " "

    $errLog = Join-Path $env:TEMP "zplatform-openocd-debug.err.log"
    $outLog = Join-Path $env:TEMP "zplatform-openocd-debug.out.log"
    Remove-Item $errLog, $outLog -ErrorAction SilentlyContinue

    $proc = Start-Process -FilePath $Tools.OpenOcd -ArgumentList $argLine -PassThru `
        -RedirectStandardError $errLog -RedirectStandardOutput $outLog -WindowStyle Hidden

    if (-not (Wait-TcpPort -Port 3333 -TimeoutSec 25)) {
        if (-not $proc.HasExited) { Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue }
        $tail = @()
        if (Test-Path $errLog) { $tail += Get-Content $errLog -Raw }
        if (Test-Path $outLog) { $tail += Get-Content $outLog -Raw }
        if (-not $tail) { $tail = @("(no log)") }
        throw "OpenOCD did not open GDB port 3333.`n$($tail -join "`n")"
    }

    Write-Host "OpenOCD GDB server ready on port 3333"
    return $proc
}

Initialize-ZPlatformEnv

if ($Kind) {
    if (-not $Board) { $Board = $script:ZPlatformDefaultBoard }
    $app = Resolve-ZPlatformApp -Kind $Kind -Name $Name -Board $Board
    $resolved = Resolve-ZPlatformBuildDir -Board $Board -Kind $app.Kind -Name $app.Name -Config $Config
    $buildDir = $resolved.BuildDir
} else {
    $meta = Read-ZPlatformLastBuild
    if (-not $meta) { throw "no previous build - build first" }
    if ($Config -and $Config -ne $meta.configuration) {
        if (-not $Board) { $Board = $meta.board }
        $resolved = Resolve-ZPlatformBuildDir -Board $Board -Kind $meta.kind -Name $meta.application -Config $Config
        $buildDir = $resolved.BuildDir
    } else {
        $buildDir = $meta.build_dir
        if (-not (Test-Path (Join-Path $buildDir "zephyr"))) {
            throw "last build missing at $buildDir - rebuild or pass demo|sample -Config"
        }
    }
}

$elf = Join-Path $buildDir "zephyr\zephyr.elf"
if (-not (Test-Path $elf)) { throw "ELF missing: $elf - build first" }

# Non-openocd runners still go through west.
if ($Runner -and $Runner -ne "openocd") {
    Write-Host "Debugging $elf (west -r $Runner)"
    $argsList = @("-d", $buildDir, "-r", $Runner)
    Invoke-ZPlatformWest debug @argsList
    exit 0
}

$tools = Get-RunnerTools -BuildDir $buildDir
Write-Host "Debugging $elf"
Write-Host "OpenOCD config: $($tools.Config)"

$ocdProc = $null
try {
    $ocdProc = Start-OpenOcdDebugServer -Tools $tools

    $gdbArgs = @(
        "-q",
        "-ex", "set confirm off",
        "-ex", "set pagination off",
        "-ex", "target extended-remote :3333",
        "-ex", "monitor reset halt"
    )
    if ($Batch) {
        $gdbArgs += @(
            "-batch",
            "-ex", "echo === DEBUG ATTACH OK ===",
            "-ex", "info registers pc",
            "-ex", "bt 4",
            "-ex", "disconnect",
            "-ex", "quit"
        )
    } else {
        Write-Host "GDB ready. Useful: c | Ctrl+C | bt | monitor reset halt | quit"
    }
    $gdbArgs += $tools.Elf

    & $tools.Gdb @gdbArgs
    if ($LASTEXITCODE -ne 0 -and $null -ne $LASTEXITCODE) {
        throw "gdb exited with $LASTEXITCODE"
    }
} finally {
    if ($ocdProc -and -not $ocdProc.HasExited) {
        Stop-Process -Id $ocdProc.Id -Force -ErrorAction SilentlyContinue
    }
    Get-Process -Name openocd -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
}
