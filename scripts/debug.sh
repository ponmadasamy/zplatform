#!/usr/bin/env bash
# Copyright (c) 2026 ZPlatform contributors
# SPDX-License-Identifier: Apache-2.0
#
# Start a debug session for the last built image (ST-Link → GDB).

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck disable=SC1091
source "${SCRIPT_DIR}/common.sh"

usage() {
	cat <<EOF
Usage: $0 [--board BOARD] [--runner openocd|jlink|...]

Uses west debug against the last successful build.
EOF
}

BOARD=""
RUNNER=""

while [[ $# -gt 0 ]]; do
	case "$1" in
	--board)
		BOARD="$2"
		shift 2
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
		exit 1
		;;
	esac
done

zplatform_setup_env
zplatform_require_cmd west
zplatform_read_last_build

BUILD_DIR="${build_dir}"
BOARD="${BOARD:-${board}}"
ELF="${BUILD_DIR}/zephyr/zephyr.elf"

if [[ ! -f "${ELF}" ]]; then
	echo "error: ELF not found: ${ELF}" >&2
	exit 1
fi

ARGS=(-d "${BUILD_DIR}")
if [[ -n "${RUNNER}" ]]; then
	ARGS+=(-r "${RUNNER}")
fi

echo "Debugging ${ELF}"
echo "Executing: west debug ${ARGS[*]}"
echo
echo "Useful GDB commands once attached:"
echo "  break main"
echo "  continue"
echo "  stepi / next / finish"
echo "  info registers"
echo "  bt"
echo "  info threads"
echo "  monitor reset halt"
echo

west debug "${ARGS[@]}"
