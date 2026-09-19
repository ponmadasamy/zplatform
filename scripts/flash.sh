#!/usr/bin/env bash
# Copyright (c) 2026 ZPlatform contributors
# SPDX-License-Identifier: Apache-2.0
#
# Flash the last built (or explicitly selected) application via west runners.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck disable=SC1091
source "${SCRIPT_DIR}/common.sh"

usage() {
	cat <<EOF
Usage:
  $0 [demo|sample <name>] [--board BOARD] [--erase] [--runner NAME]

Default: flash the last successful build recorded in build/.zplatform-last-build
EOF
}

KIND=""
NAME=""
BOARD=""
ERASE=0
RUNNER=""

while [[ $# -gt 0 ]]; do
	case "$1" in
	demo)
		KIND="demo"
		shift
		;;
	sample)
		KIND="sample"
		NAME="${2:-}"
		shift 2
		;;
	--board)
		BOARD="$2"
		shift 2
		;;
	--erase)
		ERASE=1
		shift
		;;
	--runner)
		RUNNER="$2"
		shift 2
		;;
	-h|--help)
		usage
		exit 0
		;;
	*)
		echo "error: unknown argument: $1" >&2
		usage
		exit 1
		;;
	esac
done

zplatform_setup_env
zplatform_require_cmd west

if [[ -n "${KIND}" ]]; then
	zplatform_resolve_app "${KIND}" "${NAME}"
	BOARD="${BOARD:-${ZPLATFORM_DEFAULT_BOARD}}"
	BUILD_DIR="$(zplatform_build_dir "${BOARD}" "${APP_NAME}" default)"
	if [[ ! -d "${BUILD_DIR}" ]]; then
		# try mcuboot/debug/release
		for cfg in mcuboot debug release default; do
			cand="$(zplatform_build_dir "${BOARD}" "${APP_NAME}" "${cfg}")"
			if [[ -d "${cand}/zephyr" ]]; then
				BUILD_DIR="${cand}"
				break
			fi
		done
	fi
else
	zplatform_read_last_build
	BOARD="${board}"
	BUILD_DIR="${build_dir}"
	APP_NAME="${application}"
fi

if [[ ! -d "${BUILD_DIR}" ]]; then
	echo "error: build directory not found: ${BUILD_DIR}" >&2
	exit 1
fi

echo "Flashing board=${BOARD} app=${APP_NAME} dir=${BUILD_DIR}"

# Best-effort ST-Link detection (does not fail the flash if missing).
if command -v lsusb >/dev/null 2>&1; then
	if lsusb 2>/dev/null | grep -qi "STMicroelectronics\|ST-LINK\|STM32"; then
		echo "ST-Link: detected via lsusb"
	else
		echo "ST-Link: not detected via lsusb (continuing; runner may still work)"
	fi
fi

FLASH_ARGS=(-d "${BUILD_DIR}")
if [[ "${ERASE}" -eq 1 ]]; then
	echo "Full chip erase requested"
	FLASH_ARGS+=(--erase)
fi
if [[ -n "${RUNNER}" ]]; then
	FLASH_ARGS+=(-r "${RUNNER}")
fi

echo "Executing: west flash ${FLASH_ARGS[*]}"
west flash "${FLASH_ARGS[@]}"

echo "Flash complete. Target should reset via the selected runner."
