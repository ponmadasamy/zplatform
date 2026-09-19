@echo off
setlocal EnableExtensions EnableDelayedExpansion
rem Copyright (c) 2026 ZPlatform contributors
rem SPDX-License-Identifier: Apache-2.0
rem
rem Profiling build (Windows cmd.exe).
rem
rem Usage:
rem   scripts\cmd\profile.cmd [demo|sample NAME] [--board BOARD]

set "ZP_CMD=%~dp0"
call "%ZP_CMD%common.cmd" init

set "KIND=demo"
set "NAME="
set "BOARD=%ZPLATFORM_DEFAULT_BOARD%"

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
if /i "%~1"=="--help" goto usage
echo ERROR: unknown option %~1 1>&2
goto usage

:after_parse
call "%ZP_CMD%common.cmd" setup_env
if errorlevel 1 exit /b 1

call "%ZP_CMD%common.cmd" resolve_app "%KIND%" "%NAME%"
if errorlevel 1 exit /b 1

call "%ZP_CMD%common.cmd" build_dir "%BOARD%" "%APP_NAME%" "profile"
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

set "CMAKE_ARGS=-b %BOARD% -d %BUILD_DIR% %APP_PATH% -- -DBOARD_ROOT=%ZPLATFORM_ROOT% -DZEPHYR_EXTRA_MODULES=%ZPLATFORM_ROOT% -DCONFIG_ZPLATFORM_PROFILING=y -DCONFIG_THREAD_RUNTIME_STATS=y -DCONFIG_INIT_STACKS=y -DCONFIG_THREAD_STACK_INFO=y -DCONFIG_THREAD_MONITOR=y"

set "WEST_CMDLINE=build !CMAKE_ARGS!"
call "%ZP_CMD%common.cmd" west
if errorlevel 1 (
  echo ERROR: west build failed 1>&2
  exit /b 1
)

call "%ZP_CMD%common.cmd" write_last_build "%BOARD%" "%APP_NAME%" "%BUILD_DIR%" "profile" "%APP_KIND%"
echo Profiling build ready: %BUILD_DIR%
exit /b 0

:usage
echo Usage: scripts\cmd\profile.cmd [demo^|sample NAME] [--board BOARD]
exit /b 0
