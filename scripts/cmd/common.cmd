@echo off
rem Copyright (c) 2026 ZPlatform contributors
rem SPDX-License-Identifier: Apache-2.0
rem
rem Shared helpers for scripts\cmd (Windows cmd.exe / when PowerShell is blocked).
rem Usage: call "%~dp0common.cmd" <action> [args...]
rem Actions: init, ensure_venv, setup_env, resolve_app, build_dir, app_parent_dir,
rem          remove_build, write_last_build, west

set "ZP_ACTION=%~1"
if "%ZP_ACTION%"=="" (
  echo ERROR: common.cmd requires an action 1>&2
  exit /b 2
)
shift /1
goto %ZP_ACTION%
goto :eof

:init
set "ZPLATFORM_CMD_DIR=%~dp0"
if "%ZPLATFORM_CMD_DIR:~-1%"=="\" set "ZPLATFORM_CMD_DIR=%ZPLATFORM_CMD_DIR:~0,-1%"
for %%I in ("%ZPLATFORM_CMD_DIR%\..\..") do set "ZPLATFORM_ROOT=%%~fI"
set "ZPLATFORM_ENV_PY=%ZPLATFORM_ROOT%\scripts\env\zplatform_env.py"
set "ZPLATFORM_DEFAULT_BOARD=stm3210e_eval"
set "ZPLATFORM_PY=%ZPLATFORM_ROOT%\.venv\Scripts\python.exe"
set "ZPLATFORM_WEST_EXE=%ZPLATFORM_ROOT%\.venv\Scripts\west.exe"
if exist "%ZPLATFORM_ROOT%\config\toolchain.yml" (
  for /f "usebackq tokens=2 delims=: " %%B in (`findstr /r /c:"default_board:" "%ZPLATFORM_ROOT%\config\toolchain.yml"`) do (
    set "ZPLATFORM_DEFAULT_BOARD=%%~B"
  )
)
exit /b 0

:ensure_venv
if exist "%ZPLATFORM_PY%" exit /b 0
call "%ZPLATFORM_CMD_DIR%\setup-zephyr.cmd" --ensure
exit /b %ERRORLEVEL%

:setup_env
rem Skip full --ensure when a parent script already initialized (nested builds).
if "%ZPLATFORM_ENV_READY%"=="1" if defined ZEPHYR_BASE if exist "%ZEPHYR_BASE%" exit /b 0
call "%~f0" ensure_venv
if errorlevel 1 exit /b 1
if not exist "%ZPLATFORM_PY%" (
  echo ERROR: missing "%ZPLATFORM_PY%" - run scripts\cmd\setup-zephyr.cmd first 1>&2
  exit /b 1
)
set "ZP_ENV_BAT=%TEMP%\zplatform-env-%RANDOM%.cmd"
"%ZPLATFORM_PY%" "%ZPLATFORM_ENV_PY%" --ensure --print-env-cmd > "%ZP_ENV_BAT%"
if errorlevel 1 (
  echo ERROR: environment ensure failed 1>&2
  if exist "%ZP_ENV_BAT%" del "%ZP_ENV_BAT%"
  exit /b 1
)
call "%ZP_ENV_BAT%"
del "%ZP_ENV_BAT%" >nul 2>&1
set "ZPLATFORM_ENV_READY=1"
if not defined ZEPHYR_BASE (
  echo ERROR: ZEPHYR_BASE not set 1>&2
  exit /b 1
)
exit /b 0

:resolve_app
rem Args: kind [name] [board] -> sets APP_KIND APP_NAME APP_PATH
set "APP_KIND=%~1"
set "APP_NAME="
set "APP_PATH="
set "ZP_RA_NAME=%~2"
set "ZP_RA_BOARD=%~3"
if "%ZP_RA_BOARD%"=="" set "ZP_RA_BOARD=%ZPLATFORM_DEFAULT_BOARD%"

if /i not "%APP_KIND%"=="demo" goto resolve_app_sample
set "APP_NAME=demo"
set "APP_PATH=%ZPLATFORM_ROOT%\demo\%ZP_RA_BOARD%"
if not exist "%APP_PATH%" (
  echo ERROR: no demo for board (expected %APP_PATH%^) 1>&2
  exit /b 1
)
exit /b 0

:resolve_app_sample
if /i not "%APP_KIND%"=="sample" goto resolve_app_bad
if "%ZP_RA_NAME%"=="" (
  echo ERROR: sample name required 1>&2
  exit /b 1
)
set "APP_NAME=%ZP_RA_NAME%"
set "APP_PATH=%ZPLATFORM_ROOT%\samples\%ZP_RA_NAME%"
if not exist "%APP_PATH%" (
  echo ERROR: sample not found (expected %APP_PATH%^) 1>&2
  exit /b 1
)
exit /b 0

:resolve_app_bad
echo ERROR: unknown kind "%APP_KIND%" 1>&2
exit /b 1

:build_dir
rem Args: board kind name config -> sets BUILD_DIR
rem   demo   -> build\<board>\demo\<config>
rem   sample -> build\<board>\sample\<name>\<config>
rem   boot   -> build\<board>\boot\<name>\<config>
set "BUILD_DIR="
if /i "%~2"=="demo" (
  set "BUILD_DIR=%ZPLATFORM_ROOT%\build\%~1\demo\%~4"
  exit /b 0
)
if /i "%~2"=="sample" (
  set "BUILD_DIR=%ZPLATFORM_ROOT%\build\%~1\sample\%~3\%~4"
  exit /b 0
)
if /i "%~2"=="boot" (
  if "%~3"=="" (
    set "BUILD_DIR=%ZPLATFORM_ROOT%\build\%~1\boot\mcuboot\%~4"
  ) else (
    set "BUILD_DIR=%ZPLATFORM_ROOT%\build\%~1\boot\%~3\%~4"
  )
  exit /b 0
)
echo ERROR: unknown kind "%~2" for build_dir 1>&2
exit /b 1

:app_parent_dir
rem Args: board kind name -> sets APP_PARENT
set "APP_PARENT="
if /i "%~2"=="demo" (
  set "APP_PARENT=%ZPLATFORM_ROOT%\build\%~1\demo"
  exit /b 0
)
if /i "%~2"=="sample" (
  set "APP_PARENT=%ZPLATFORM_ROOT%\build\%~1\sample\%~3"
  exit /b 0
)
if /i "%~2"=="boot" (
  if "%~3"=="" (
    set "APP_PARENT=%ZPLATFORM_ROOT%\build\%~1\boot\mcuboot"
  ) else (
    set "APP_PARENT=%ZPLATFORM_ROOT%\build\%~1\boot\%~3"
  )
  exit /b 0
)
echo ERROR: unknown kind "%~2" for app_parent_dir 1>&2
exit /b 1

:remove_build
rem Args modes (set before call):
rem   ZP_CLEAN_ALL=1
rem   ZP_CLEAN_BOARD=<board>   (board-wide; leave KIND empty)
rem   ZP_CLEAN_BOARD + ZP_CLEAN_KIND + ZP_CLEAN_NAME [+ ZP_CLEAN_CFG]
if defined ZP_CLEAN_ALL (
  if exist "%ZPLATFORM_ROOT%\build" (
    echo Cleaning all under %ZPLATFORM_ROOT%\build
    for /d %%D in ("%ZPLATFORM_ROOT%\build\*") do rd /s /q "%%~fD" 2>nul
    if exist "%ZPLATFORM_ROOT%\build\.zplatform-last-build" del /q "%ZPLATFORM_ROOT%\build\.zplatform-last-build"
  ) else (
    echo Nothing to clean (%ZPLATFORM_ROOT%\build missing^)
  )
  set "ZP_CLEAN_ALL="
  exit /b 0
)
if not defined ZP_CLEAN_BOARD (
  echo ERROR: ZP_CLEAN_BOARD required unless ZP_CLEAN_ALL 1>&2
  exit /b 1
)
if not defined ZP_CLEAN_KIND (
  if exist "%ZPLATFORM_ROOT%\build\%ZP_CLEAN_BOARD%" (
    echo Cleaning %ZPLATFORM_ROOT%\build\%ZP_CLEAN_BOARD%
    rd /s /q "%ZPLATFORM_ROOT%\build\%ZP_CLEAN_BOARD%"
  )
  set "ZP_CLEAN_BOARD="
  exit /b 0
)
if defined ZP_CLEAN_CFG goto remove_build_cfg
call "%~f0" app_parent_dir "%ZP_CLEAN_BOARD%" "%ZP_CLEAN_KIND%" "%ZP_CLEAN_NAME%"
if exist "%APP_PARENT%" (
  echo Cleaning %APP_PARENT%
  rd /s /q "%APP_PARENT%"
)
goto remove_build_done

:remove_build_cfg
call "%~f0" build_dir "%ZP_CLEAN_BOARD%" "%ZP_CLEAN_KIND%" "%ZP_CLEAN_NAME%" "%ZP_CLEAN_CFG%"
if exist "%BUILD_DIR%" (
  echo Cleaning %BUILD_DIR%
  rd /s /q "%BUILD_DIR%"
)

:remove_build_done
set "ZP_CLEAN_BOARD="
set "ZP_CLEAN_KIND="
set "ZP_CLEAN_NAME="
set "ZP_CLEAN_CFG="
exit /b 0

:write_last_build
rem Args: board app build_dir config kind
set "META_DIR=%ZPLATFORM_ROOT%\build"
if not exist "%META_DIR%" mkdir "%META_DIR%"
(
  echo board=%~1
  echo kind=%~5
  echo application=%~2
  echo configuration=%~4
  echo build_dir=%~3
) > "%META_DIR%\.zplatform-last-build"
exit /b 0

:west
rem Caller must set WEST_CMDLINE (e.g. build -b ...)
if not exist "%ZPLATFORM_WEST_EXE%" (
  echo ERROR: missing "%ZPLATFORM_WEST_EXE%" 1>&2
  exit /b 1
)
if not defined ZPLATFORM_WEST_WORKSPACE (
  echo ERROR: ZPLATFORM_WEST_WORKSPACE not set - call setup_env first 1>&2
  exit /b 1
)
if not defined WEST_CMDLINE (
  echo ERROR: WEST_CMDLINE not set 1>&2
  exit /b 1
)
pushd "%ZPLATFORM_ROOT%"
"%ZPLATFORM_WEST_EXE%" %WEST_CMDLINE%
set "ERR=%ERRORLEVEL%"
popd
set "WEST_CMDLINE="
exit /b %ERR%
