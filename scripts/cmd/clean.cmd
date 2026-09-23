@echo off
setlocal EnableExtensions EnableDelayedExpansion
rem Copyright (c) 2026 ZPlatform contributors
rem SPDX-License-Identifier: Apache-2.0
rem
rem Clean ZPlatform build outputs.
rem
rem   scripts\cmd\clean.cmd --all
rem   scripts\cmd\clean.cmd --board stm3210e_eval
rem   scripts\cmd\clean.cmd demo --board stm3210e_eval --config default
rem   scripts\cmd\clean.cmd sample gpio --board stm3210e_eval

set "ZP_CMD=%~dp0"
call "%ZP_CMD%common.cmd" init

if "%~1"=="" goto usage
if /i "%~1"=="--help" goto usage
if /i "%~1"=="-h" goto usage

set "ALL="
set "BOARD="
set "KIND="
set "NAME="
set "CFG="

if /i "%~1"=="--all" (
  set "ALL=1"
  shift
  goto after_kind
)
if /i "%~1"=="--board" goto take_board_only
if /i "%~1"=="demo" goto take_demo
if /i "%~1"=="sample" goto take_sample
if /i "%~1"=="boot" goto take_boot
echo ERROR: unknown argument %~1 1>&2
goto usage

:take_board_only
set "BOARD=%~2"
if not defined BOARD (
  echo ERROR: --board requires a name 1>&2
  exit /b 1
)
shift
shift
goto after_kind

:take_demo
set "KIND=demo"
set "NAME=demo"
shift
goto parse

:take_sample
set "KIND=sample"
shift
if "%~1"=="" (
  echo ERROR: sample name required 1>&2
  exit /b 1
)
set "NAME=%~1"
shift
goto parse

:take_boot
set "KIND=boot"
set "NAME=mcuboot"
shift
if "%~1"=="" goto parse
echo %~1| findstr /b /c:"--" >nul
if errorlevel 1 (
  set "NAME=%~1"
  shift
)
goto parse

:parse
if "%~1"=="" goto after_kind
if /i "%~1"=="--board" (
  set "BOARD=%~2"
  shift
  shift
  goto parse
)
if /i "%~1"=="--config" (
  set "CFG=%~2"
  shift
  shift
  goto parse
)
if /i "%~1"=="--help" goto usage
if /i "%~1"=="-h" goto usage
echo ERROR: unknown option %~1 1>&2
goto usage

:after_kind
if defined ALL goto do_all
if not defined BOARD set "BOARD=%ZPLATFORM_DEFAULT_BOARD%"
if not defined KIND goto do_board

set "ZP_CLEAN_BOARD=%BOARD%"
set "ZP_CLEAN_KIND=%KIND%"
set "ZP_CLEAN_NAME=%NAME%"
if defined CFG set "ZP_CLEAN_CFG=%CFG%"
call "%ZP_CMD%common.cmd" remove_build
if defined CFG (
  echo Cleaned board=%BOARD% %KIND%/%NAME%/%CFG%
) else (
  echo Cleaned board=%BOARD% %KIND%/%NAME% all-configs
)
exit /b 0

:do_all
if defined KIND (
  echo ERROR: --all cannot be combined with kind / --board / --config 1>&2
  exit /b 1
)
if defined BOARD (
  echo ERROR: --all cannot be combined with kind / --board / --config 1>&2
  exit /b 1
)
if defined CFG (
  echo ERROR: --all cannot be combined with kind / --board / --config 1>&2
  exit /b 1
)
set "ZP_CLEAN_ALL=1"
call "%ZP_CMD%common.cmd" remove_build
echo Cleaned all.
exit /b 0

:do_board
set "ZP_CLEAN_BOARD=%BOARD%"
call "%ZP_CMD%common.cmd" remove_build
echo Cleaned board=%BOARD%
exit /b 0

:usage
echo Usage:
echo   scripts\cmd\clean.cmd --all
echo   scripts\cmd\clean.cmd --board BOARD
echo   scripts\cmd\clean.cmd demo [--board BOARD] [--config CFG]
echo   scripts\cmd\clean.cmd sample NAME [--board BOARD] [--config CFG]
echo   scripts\cmd\clean.cmd boot [mcuboot] [--board BOARD] [--config CFG]
echo.
echo Before-build: scripts\cmd\build.cmd demo --board BOARD --clean
exit /b 0
