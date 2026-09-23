#!/usr/bin/env bash
# Copyright (c) 2026 ZPlatform contributors
# SPDX-License-Identifier: Apache-2.0
#
# High-level standalone demo workflow (no MCUboot).
#
#   ./scripts/sh/demo.sh --board stm3210e_eval
#   ./scripts/sh/demo.sh --board stm3210e_eval --flash
#   ./scripts/sh/demo.sh --build --flash --debug

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck disable=SC1091
source "${SCRIPT_DIR}/common.sh"

usage() {
	cat <<EOF
Usage:
  $0 [--board BOARD] [--build] [--flash] [--debug]
     [--clean|--clean-all] [--debug-cfg|--release]

Standalone per-board demo (demo/<board>/). Default action: --build.

  --clean       Before build: remove this config's tree only
  --clean-all   Before build: remove entire build/<board>/

Standalone clean:
  ./scripts/sh/clean.sh demo --board BOARD --config default
  ./scripts/sh/clean.sh --board BOARD
  ./scripts/sh/clean.sh --all

MCUboot flow: ./scripts/sh/mcuboot_demo.sh --help
EOF
}

BOARD="${ZPLATFORM_DEFAULT_BOARD}"
DO_BUILD=0
DO_FLASH=0
DO_DEBUG=0
CLEAN=0
CLEAN_ALL=0
DEBUG_CFG=0
RELEASE=0

while [[ $# -gt 0 ]]; do
	case "$1" in
	--board) BOARD="$2"; shift 2 ;;
	--build) DO_BUILD=1; shift ;;
	--flash) DO_FLASH=1; shift ;;
	--debug) DO_DEBUG=1; shift ;;
	--clean) CLEAN=1; shift ;;
	--clean-all) CLEAN_ALL=1; shift ;;
	--debug-cfg) DEBUG_CFG=1; shift ;;
	--release) RELEASE=1; shift ;;
	-h|--help) usage; exit 0 ;;
	*) echo "error: unknown argument: $1" >&2; usage; exit 1 ;;
	esac
done

if [[ "${CLEAN}" -eq 1 && "${CLEAN_ALL}" -eq 1 ]]; then
	echo "error: use only one of --clean or --clean-all" >&2
	exit 1
fi

if [[ "${DO_BUILD}" -eq 0 && "${DO_FLASH}" -eq 0 && "${DO_DEBUG}" -eq 0 ]]; then
	DO_BUILD=1
fi

CFG="default"
EXTRA=()
if [[ "${DEBUG_CFG}" -eq 1 ]]; then CFG="debug"; EXTRA+=(--debug); fi
if [[ "${RELEASE}" -eq 1 ]]; then CFG="release"; EXTRA+=(--release); fi

zplatform_resolve_app demo demo "${BOARD}"

if [[ "${CLEAN_ALL}" -eq 1 ]]; then
	zplatform_remove_build --board "${BOARD}"
elif [[ "${CLEAN}" -eq 1 && "${DO_BUILD}" -eq 0 ]]; then
	zplatform_remove_build --board "${BOARD}" --kind demo --name demo --config "${CFG}"
	echo "demo.sh cleaned board=${BOARD} config=${CFG} (no build)."
	exit 0
fi

if [[ "${DO_BUILD}" -eq 1 ]]; then
	echo "=== demo: build (config=${CFG}, board=${BOARD}) ==="
	ARGS=(demo --board "${BOARD}" "${EXTRA[@]}")
	if [[ "${CLEAN}" -eq 1 && "${CLEAN_ALL}" -eq 0 ]]; then ARGS+=(--clean); fi
	"${SCRIPT_DIR}/build.sh" "${ARGS[@]}"
fi

if [[ "${DO_FLASH}" -eq 1 ]]; then
	BUILD_DIR="$(zplatform_build_dir "${BOARD}" demo demo "${CFG}")"
	if [[ ! -d "${BUILD_DIR}/zephyr" ]]; then
		echo "=== demo: no ${CFG} tree — building first ==="
		ARGS=(demo --board "${BOARD}" "${EXTRA[@]}")
		"${SCRIPT_DIR}/build.sh" "${ARGS[@]}"
	fi
	echo "=== demo: flash (config=${CFG}, board=${BOARD}) ==="
	"${SCRIPT_DIR}/flash.sh" demo --board "${BOARD}" --config "${CFG}"
fi

if [[ "${DO_DEBUG}" -eq 1 ]]; then
	echo "=== demo: debug (config=${CFG}, board=${BOARD}) ==="
	"${SCRIPT_DIR}/debug.sh" demo --board "${BOARD}" --config "${CFG}"
fi

echo "demo.sh done (board=${BOARD} config=${CFG})."
