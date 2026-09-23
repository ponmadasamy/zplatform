@echo off
setlocal EnableExtensions EnableDelayedExpansion
rem Copyright (c) 2026 ZPlatform contributors
rem SPDX-License-Identifier: Apache-2.0
rem
rem ZPlatform Zephyr environment manager (Windows cmd.exe).
rem Use when PowerShell script execution is blocked.
rem
rem Usage:
rem   scripts\cmd\setup-zephyr.cmd [--status] [--ensure] [--sync] ...

set "ZP_CMD=%~dp0"
call "%ZP_CMD%common.cmd" init

set "DO_STATUS="
set "DO_ENSURE="
set "DO_SYNC="
set "DO_SYNC_MANIFEST="
set "DO_CHECK_LATEST="
set "DO_UPDATE="
set "DO_HELP="

:parse
if "%~1"=="" goto after_parse
if /i "%~1"=="--status" set "DO_STATUS=1" & shift & goto parse
if /i "%~1"=="--ensure" set "DO_ENSURE=1" & shift & goto parse
if /i "%~1"=="--sync" set "DO_SYNC=1" & shift & goto parse
if /i "%~1"=="--sync-manifest" set "DO_SYNC_MANIFEST=1" & shift & goto parse
if /i "%~1"=="--check-latest" set "DO_CHECK_LATEST=1" & shift & goto parse
if /i "%~1"=="--update" set "DO_UPDATE=1" & shift & goto parse
if /i "%~1"=="--help" set "DO_HELP=1" & shift & goto parse
if /i "%~1"=="-h" set "DO_HELP=1" & shift & goto parse
echo ERROR: unknown option %~1 1>&2
exit /b 2

:after_parse
if defined DO_HELP (
  echo Usage: scripts\cmd\setup-zephyr.cmd [options]
  echo.
  echo   --status           Show environment status
  echo   --ensure           Create/validate environment
  echo   --sync             west update to pinned Zephyr revision
  echo   --sync-manifest    Copy toolchain.yml revision into west.yml
  echo   --check-latest     Query official Zephyr releases ^(no pin change^)
  echo   --update           Same as --check-latest
  exit /b 0
)

if not exist "%ZPLATFORM_PY%" (
  set "BOOT_PY="
  where py >nul 2>&1 && set "BOOT_PY=py -3"
  if not defined BOOT_PY (
    where python >nul 2>&1 && set "BOOT_PY=python"
  )
  if not defined BOOT_PY (
    echo ERROR: Python ^>= 3.12 required. Install from python.org or use py -3.12. 1>&2
    exit /b 1
  )
  echo Bootstrapping .venv with %BOOT_PY%
  %BOOT_PY% -m venv "%ZPLATFORM_ROOT%\.venv"
  if errorlevel 1 exit /b 1
  rem Pip/requirements: single path via zplatform_env.py --ensure (hash-stamped).
)

set "PY_ARGS=%ZPLATFORM_ENV_PY%"
if defined DO_STATUS set "PY_ARGS=!PY_ARGS! --status"
if defined DO_ENSURE set "PY_ARGS=!PY_ARGS! --ensure"
if defined DO_SYNC set "PY_ARGS=!PY_ARGS! --sync"
if defined DO_SYNC_MANIFEST set "PY_ARGS=!PY_ARGS! --sync-manifest"
if defined DO_CHECK_LATEST set "PY_ARGS=!PY_ARGS! --check-latest"
if defined DO_UPDATE set "PY_ARGS=!PY_ARGS! --update"

if not defined DO_STATUS if not defined DO_ENSURE if not defined DO_SYNC if not defined DO_SYNC_MANIFEST if not defined DO_CHECK_LATEST if not defined DO_UPDATE (
  set "PY_ARGS=!PY_ARGS! --ensure --status"
)

"%ZPLATFORM_PY%" !PY_ARGS!
exit /b %ERRORLEVEL%
