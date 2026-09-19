@echo off
setlocal EnableExtensions EnableDelayedExpansion
rem Copyright (c) 2026 ZPlatform contributors
rem SPDX-License-Identifier: Apache-2.0
rem
rem Run Twister / Ztest (Windows cmd.exe).
rem
rem Usage:
rem   scripts\cmd\test.cmd [platform/smoke] [--board BOARD] [--native]

set "ZP_CMD=%~dp0"
call "%ZP_CMD%common.cmd" init

set "FOCUS="
set "BOARD=%ZPLATFORM_DEFAULT_BOARD%"
set "NATIVE="

if /i "%~1"=="--help" goto usage
if /i "%~1"=="-h" goto usage

if not "%~1"=="" if /i not "%~1:~0,2%"=="--" (
  set "FOCUS=%~1"
  shift
)

:parse
if "%~1"=="" goto after_parse
if /i "%~1"=="--board" set "BOARD=%~2" & shift & shift & goto parse
if /i "%~1"=="--native" set "NATIVE=1" & shift & goto parse
if /i "%~1"=="--help" goto usage
echo ERROR: unknown option %~1 1>&2
goto usage

:after_parse
call "%ZP_CMD%common.cmd" setup_env
if errorlevel 1 exit /b 1

set "TEST_ROOT=%ZPLATFORM_ROOT%\tests"
if defined FOCUS set "TEST_ROOT=%TEST_ROOT%\%FOCUS%"
if not exist "%TEST_ROOT%" (
  echo ERROR: test path not found: %TEST_ROOT% 1>&2
  exit /b 1
)

set "OUTDIR=%ZPLATFORM_ROOT%\build\twister"
if not exist "%OUTDIR%" mkdir "%OUTDIR%"

echo ========================================
echo ZPlatform Twister
echo ========================================
echo ZEPHYR_BASE : %ZEPHYR_BASE%
echo Workspace   : %ZPLATFORM_WEST_WORKSPACE%
echo west        : %ZPLATFORM_WEST%
echo ========================================

set "TW_ARGS=-T %TEST_ROOT% -O %OUTDIR% -v --board-root %ZPLATFORM_ROOT% -x=-DZEPHYR_EXTRA_MODULES=%ZPLATFORM_ROOT% -x=-DBOARD_ROOT=%ZPLATFORM_ROOT%"
if defined NATIVE (
  set "TW_ARGS=!TW_ARGS! -p native_sim"
) else (
  set "TW_ARGS=!TW_ARGS! -p %BOARD% --build-only"
  echo Note: default mode is --build-only ^(no hardware execution^).
)

set "WEST_CMDLINE=twister !TW_ARGS!"
call "%ZP_CMD%common.cmd" west
exit /b %ERRORLEVEL%

:usage
echo Usage: scripts\cmd\test.cmd [platform/smoke] [--board BOARD] [--native]
exit /b 0
