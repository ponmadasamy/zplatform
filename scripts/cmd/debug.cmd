@echo off
setlocal EnableExtensions EnableDelayedExpansion
rem Copyright (c) 2026 ZPlatform contributors
rem SPDX-License-Identifier: Apache-2.0
rem
rem Debug the last ZPlatform build (Windows cmd.exe).
rem
rem Usage:
rem   scripts\cmd\debug.cmd [--runner NAME]

set "ZP_CMD=%~dp0"
call "%ZP_CMD%common.cmd" init

set "RUNNER="

:parse
if "%~1"=="" goto after_parse
if /i "%~1"=="--runner" set "RUNNER=%~2" & shift & shift & goto parse
if /i "%~1"=="--board" shift & shift & goto parse
if /i "%~1"=="--help" goto usage
if /i "%~1"=="-h" goto usage
echo ERROR: unknown option %~1 1>&2
goto usage

:after_parse
call "%ZP_CMD%common.cmd" setup_env
if errorlevel 1 exit /b 1

set "META=%ZPLATFORM_ROOT%\build\.zplatform-last-build"
if not exist "%META%" (
  echo ERROR: no previous build - build first 1>&2
  exit /b 1
)
for /f "usebackq tokens=1,* delims==" %%A in ("%META%") do (
  if /i "%%A"=="build_dir" set "BUILD_DIR=%%B"
)

set "DBG_ARGS=-d %BUILD_DIR%"
if defined RUNNER set "DBG_ARGS=!DBG_ARGS! -r %RUNNER%"

echo Debugging %BUILD_DIR%\zephyr\zephyr.elf
set "WEST_CMDLINE=debug !DBG_ARGS!"
call "%ZP_CMD%common.cmd" west
exit /b %ERRORLEVEL%

:usage
echo Usage: scripts\cmd\debug.cmd [--runner NAME]
exit /b 0
