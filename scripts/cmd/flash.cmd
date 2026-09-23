@echo off
setlocal EnableExtensions EnableDelayedExpansion
rem Copyright (c) 2026 ZPlatform contributors
rem SPDX-License-Identifier: Apache-2.0
rem
rem Flash an existing ZPlatform build (never rebuilds).
rem
rem Usage:
rem   scripts\cmd\flash.cmd [demo|sample NAME] [--board BOARD] [--config CFG] [--erase] [--runner NAME]

set "ZP_CMD=%~dp0"
call "%ZP_CMD%common.cmd" init

set "KIND="
set "NAME="
set "BOARD="
set "CONFIG="
set "ERASE="
set "RUNNER="
set "APP_DISP="
set "BUILD_DIR="
set "CFG="

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
  if /i "!KIND!"=="demo" set "NAME=demo"
)

:parse
if "%~1"=="" goto after_parse
if /i "%~1"=="--board" set "BOARD=%~2" & shift & shift & goto parse
if /i "%~1"=="--config" set "CONFIG=%~2" & shift & shift & goto parse
if /i "%~1"=="--erase" set "ERASE=1" & shift & goto parse
if /i "%~1"=="--runner" set "RUNNER=%~2" & shift & shift & goto parse
if /i "%~1"=="--help" goto usage
echo ERROR: unknown option %~1 1>&2
goto usage

:after_parse
call "%ZP_CMD%common.cmd" setup_env
if errorlevel 1 exit /b 1

"%ZPLATFORM_PY%" "%ZPLATFORM_ENV_PY%" --require-flash-tools
if errorlevel 1 exit /b 1

if defined KIND (
  if not defined BOARD set "BOARD=%ZPLATFORM_DEFAULT_BOARD%"
  call "%ZP_CMD%common.cmd" resolve_app "%KIND%" "%NAME%" "!BOARD!"
  if errorlevel 1 exit /b 1
  if not defined CONFIG set "CONFIG=default"
  call "%ZP_CMD%common.cmd" build_dir "!BOARD!" "!APP_KIND!" "!APP_NAME!" "!CONFIG!"
  if not exist "!BUILD_DIR!\zephyr" (
    echo ERROR: no build at !BUILD_DIR! — build first or pass --config 1>&2
    exit /b 1
  )
  set "APP_DISP=!APP_NAME!"
  set "CFG=!CONFIG!"
) else (
  set "META=%ZPLATFORM_ROOT%\build\.zplatform-last-build"
  if not exist "!META!" (
    echo ERROR: no previous build metadata 1>&2
    exit /b 1
  )
  for /f "usebackq tokens=1,* delims==" %%A in ("!META!") do (
    if /i "%%A"=="board" set "BOARD=%%B"
    if /i "%%A"=="build_dir" set "BUILD_DIR=%%B"
    if /i "%%A"=="application" set "APP_DISP=%%B"
    if /i "%%A"=="configuration" set "CFG=%%B"
    if /i "%%A"=="kind" set "KIND=%%B"
  )
  if defined CONFIG if /i not "!CONFIG!"=="!CFG!" (
    call "%ZP_CMD%common.cmd" build_dir "!BOARD!" "!KIND!" "!APP_DISP!" "!CONFIG!"
    set "CFG=!CONFIG!"
  )
  if not exist "!BUILD_DIR!\zephyr" (
    echo ERROR: last build missing at !BUILD_DIR! 1>&2
    exit /b 1
  )
)

set "FLASH_ARGS=-d %BUILD_DIR%"
if defined ERASE set "FLASH_ARGS=!FLASH_ARGS! --erase"
if defined RUNNER set "FLASH_ARGS=!FLASH_ARGS! -r %RUNNER%"

echo Flashing board=%BOARD% app=%APP_DISP% config=%CFG% dir=%BUILD_DIR%
set "WEST_CMDLINE=flash !FLASH_ARGS!"
call "%ZP_CMD%common.cmd" west
if errorlevel 1 (
  echo ERROR: west flash failed 1>&2
  exit /b 1
)
echo Flash complete.
exit /b 0

:usage
echo Usage: scripts\cmd\flash.cmd [demo^|sample NAME] [--board BOARD] [--config CFG] [--erase] [--runner NAME]
exit /b 0
