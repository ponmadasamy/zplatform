@echo off
rem Copyright (c) 2026 ZPlatform contributors
rem SPDX-License-Identifier: Apache-2.0
rem
rem Shared helpers for scripts\cmd (Windows cmd.exe / when PowerShell is blocked).
rem Usage: call "%~dp0common.cmd" <action> [args...]
rem Actions: init, ensure_venv, setup_env, resolve_app, build_dir, write_last_build, west

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
if not defined ZEPHYR_BASE (
  echo ERROR: ZEPHYR_BASE not set 1>&2
  exit /b 1
)
exit /b 0

:resolve_app
rem Args: kind [name] -> sets APP_KIND APP_NAME APP_PATH
set "APP_KIND=%~1"
set "APP_NAME="
set "APP_PATH="
if /i "%APP_KIND%"=="demo" (
  set "APP_NAME=demo"
  set "APP_PATH=%ZPLATFORM_ROOT%\demo\zplatform_demo"
  exit /b 0
)
if /i "%APP_KIND%"=="sample" (
  if "%~2"=="" (
    echo ERROR: sample name required 1>&2
    exit /b 1
  )
  set "APP_NAME=%~2"
  set "APP_PATH=%ZPLATFORM_ROOT%\samples\%~2"
  exit /b 0
)
echo ERROR: unknown kind "%APP_KIND%" 1>&2
exit /b 1

:build_dir
rem Args: board app config -> sets BUILD_DIR
set "BUILD_DIR=%ZPLATFORM_ROOT%\build\%~1\%~2\%~3"
exit /b 0

:write_last_build
rem Args: board app build_dir config kind
set "META_DIR=%ZPLATFORM_ROOT%\build"
if not exist "%META_DIR%" mkdir "%META_DIR%"
(
  echo board=%~1
  echo application=%~2
  echo build_dir=%~3
  echo configuration=%~4
  echo kind=%~5
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
