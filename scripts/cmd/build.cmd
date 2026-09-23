@echo off
setlocal EnableExtensions EnableDelayedExpansion
rem Copyright (c) 2026 ZPlatform contributors
rem SPDX-License-Identifier: Apache-2.0
rem
rem Build a ZPlatform demo or sample (Windows cmd.exe).
rem
rem Usage:
rem   scripts\cmd\build.cmd demo [--board BOARD] [--clean] [--debug|--release] [--mcuboot]
rem   scripts\cmd\build.cmd sample board_bringup [--board BOARD] ...

set "ZP_CMD=%~dp0"
call "%ZP_CMD%common.cmd" init

if "%~1"=="" goto usage
if /i "%~1"=="--help" goto usage
if /i "%~1"=="-h" goto usage

set "KIND=%~1"
shift
set "NAME="
if /i "%KIND%"=="sample" (
  if "%~1"=="" (
    echo ERROR: sample requires a name 1>&2
    exit /b 1
  )
  if /i "%~1:~0,2%"=="--" (
    echo ERROR: sample requires a name 1>&2
    exit /b 1
  )
  set "NAME=%~1"
  shift
)

set "BOARD=%ZPLATFORM_DEFAULT_BOARD%"
set "CLEAN="
set "CFG=default"
set "MCUBOOT="
set "EXTRA="

:parse
if "%~1"=="" goto after_parse
if /i "%~1"=="--board" (
  set "BOARD=%~2"
  shift & shift & goto parse
)
if /i "%~1"=="--clean" set "CLEAN=1" & shift & goto parse
if /i "%~1"=="--debug" (
  set "CFG=debug"
  set "EXTRA=-DCONFIG_DEBUG=y -DCONFIG_DEBUG_OPTIMIZATIONS=y -DCONFIG_THREAD_NAME=y"
  shift & goto parse
)
if /i "%~1"=="--release" (
  set "CFG=release"
  set "EXTRA=-DCONFIG_SIZE_OPTIMIZATIONS=y"
  shift & goto parse
)
if /i "%~1"=="--mcuboot" (
  set "MCUBOOT=1"
  set "CFG=mcuboot"
  shift & goto parse
)
if /i "%~1"=="--help" goto usage
if /i "%~1"=="-h" goto usage
echo ERROR: unknown option %~1 1>&2
goto usage

:after_parse
call "%ZP_CMD%common.cmd" setup_env
if errorlevel 1 exit /b 1

call "%ZP_CMD%common.cmd" resolve_app "%KIND%" "%NAME%" "%BOARD%"
if errorlevel 1 exit /b 1

call "%ZP_CMD%common.cmd" build_dir "%BOARD%" "%APP_KIND%" "%APP_NAME%" "%CFG%"

"%ZPLATFORM_PY%" "%ZPLATFORM_ENV_PY%" --banner --board "%BOARD%" --application "%APP_NAME%" --config-name "%CFG%"
if errorlevel 1 exit /b 1

if defined CLEAN (
  set "ZP_CLEAN_BOARD=%BOARD%"
  set "ZP_CLEAN_KIND=%APP_KIND%"
  set "ZP_CLEAN_NAME=%APP_NAME%"
  set "ZP_CLEAN_CFG=%CFG%"
  call "%ZP_CMD%common.cmd" remove_build
)
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

rem CMake/DTS path args must use forward slashes (backslash escapes break DTC_OVERLAY_FILE).
set "ROOT_FWD=%ZPLATFORM_ROOT:\=/%"
set "BUILD_FWD=%BUILD_DIR:\=/%"
set "APP_FWD=%APP_PATH:\=/%"

set "CMAKE_ARGS=-b %BOARD% -d %BUILD_FWD% %APP_FWD% -- -DBOARD_ROOT=%ROOT_FWD% -DZEPHYR_EXTRA_MODULES=%ROOT_FWD%"
if defined EXTRA set "CMAKE_ARGS=!CMAKE_ARGS! !EXTRA!"
if defined MCUBOOT (
  set "OVERLAY=%ZPLATFORM_ROOT%\boards\st\%BOARD%\%BOARD%_mcuboot.overlay"
  if not exist "!OVERLAY!" (
    echo ERROR: MCUboot overlay not found for board %BOARD% 1>&2
    exit /b 1
  )
  if not exist "%ZPLATFORM_ROOT%\keys\root-ec-p256.pem" (
    echo ERROR: missing keys\root-ec-p256.pem - see keys\README.md 1>&2
    exit /b 1
  )
  set "OVERLAY_FWD=!OVERLAY:\=/!"
  set "CMAKE_ARGS=!CMAKE_ARGS! -DZPLATFORM_ROOT=%ROOT_FWD% -DCONFIG_BOOTLOADER_MCUBOOT=y -DCONFIG_STREAM_FLASH=y -DCONFIG_IMG_MANAGER=y -DCONFIG_MCUBOOT_IMG_MANAGER=y -DDTC_OVERLAY_FILE=!OVERLAY_FWD!"
  if exist "%APP_PATH%\prj_mcuboot.conf" (
    set "EXTRA_CONF_FWD=%APP_PATH:\=/%/prj_mcuboot.conf"
    set "CMAKE_ARGS=!CMAKE_ARGS! -DEXTRA_CONF_FILE=!EXTRA_CONF_FWD!"
  )
)

echo Executing: west build !CMAKE_ARGS!
set "WEST_CMDLINE=build !CMAKE_ARGS!"
call "%ZP_CMD%common.cmd" west
if errorlevel 1 (
  echo ERROR: west build failed 1>&2
  exit /b 1
)

call "%ZP_CMD%common.cmd" write_last_build "%BOARD%" "%APP_NAME%" "%BUILD_DIR%" "%CFG%" "%APP_KIND%"
"%ZPLATFORM_PY%" "%ZPLATFORM_ENV_PY%" --write-fingerprint "%BUILD_DIR%" --board "%BOARD%" --application "%APP_NAME%"
"%ZPLATFORM_PY%" "%ZPLATFORM_ENV_PY%" --print-layout "%BUILD_DIR%"

echo Build complete: %BUILD_DIR%
exit /b 0

:usage
echo Usage:
echo   scripts\cmd\build.cmd demo [--board BOARD] [--clean] [--debug^|--release] [--mcuboot]
echo   scripts\cmd\build.cmd sample board_bringup [--board BOARD] ...
echo.
echo   --mcuboot   Link into MCUboot slot0 (see boards\...\stm3210e_eval_mcuboot.overlay^)
echo   Partitions: boards\st\stm3210e_eval\stm3210e_eval.dts
exit /b 1
