@echo off
setlocal EnableExtensions EnableDelayedExpansion
rem Copyright (c) 2026 ZPlatform contributors
rem SPDX-License-Identifier: Apache-2.0
rem
rem Debug an existing ZPlatform build (never rebuilds).
rem Delegates to scripts\ps1\debug.ps1 (OpenOCD wait-for-port + GDB).
rem
rem Usage:
rem   scripts\cmd\debug.cmd [demo|sample NAME] [--board BOARD] [--config CFG] [--runner NAME] [--batch]

set "ZP_CMD=%~dp0"
call "%ZP_CMD%common.cmd" init

set "KIND="
set "NAME="
set "BOARD="
set "CONFIG="
set "RUNNER="
set "BATCH="

if /i "%~1"=="--help" goto usage
if /i "%~1"=="-h" goto usage

if not "%~1"=="" if /i not "%~1:~0,2%"=="--" (
  set "KIND=%~1"
  shift
  if /i "!KIND!"=="sample" (
    if "%~1"=="" (
      echo ERROR: sample name required 1>&2
      exit /b 1
    )
    set "NAME=%~1"
    shift
  )
)

:parse
if "%~1"=="" goto after_parse
if /i "%~1"=="--board" set "BOARD=%~2" & shift & shift & goto parse
if /i "%~1"=="--config" set "CONFIG=%~2" & shift & shift & goto parse
if /i "%~1"=="--runner" set "RUNNER=%~2" & shift & shift & goto parse
if /i "%~1"=="--batch" set "BATCH=1" & shift & goto parse
if /i "%~1"=="--help" goto usage
echo ERROR: unknown option %~1 1>&2
goto usage

:after_parse
set "PS=%ZPLATFORM_ROOT%\scripts\ps1\debug.ps1"

rem Assemble argv for powershell -File (each token separate).
set "A="
if defined KIND set "A=!A! !KIND!"
if defined NAME set "A=!A! !NAME!"
if defined BOARD set "A=!A! -Board !BOARD!"
if defined CONFIG set "A=!A! -Config !CONFIG!"
if defined RUNNER set "A=!A! -Runner !RUNNER!"
if defined BATCH set "A=!A! -Batch"

powershell -NoProfile -ExecutionPolicy Bypass -File "%PS%"!A!
exit /b %ERRORLEVEL%

:usage
echo Usage: scripts\cmd\debug.cmd [demo^|sample NAME] [--board BOARD] [--config CFG] [--runner NAME] [--batch]
echo.
echo  Starts OpenOCD, waits for GDB port 3333, then attaches GDB.
echo  --batch  Non-interactive smoke attach (print pc/bt and quit).
exit /b 0
