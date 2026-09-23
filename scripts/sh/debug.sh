#!/usr/bin/env bash
# Copyright (c) 2026 ZPlatform contributors
# SPDX-License-Identifier: Apache-2.0
#
# Debug an existing build. Never rebuilds.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck disable=SC1091
source "${SCRIPT_DIR}/common.sh"

usage() {
	cat <<EOF
Usage:
  $0 [demo|sample <name>] [--board BOARD] [--config CFG] [--runner NAME]

  No app args: debug the last successful build.
EOF
}

KIND=""
NAME=""
BOARD=""
CONFIG=""
RUNNER=""

while [[ $# -gt 0 ]]; do
	case "$1" in
	demo)
		KIND="demo"
		NAME="demo"
		shift
		;;
	sample)
		KIND="sample"
		NAME="${2:-}"
		if [[ -z "${NAME}" || "${NAME}" == --* ]]; then
			echo "error: sample name required" >&2
			exit 1
		fi
		shift 2
		;;
	--board)
		BOARD="$2"
		shift 2
		;;
	--config)
		CONFIG="$2"
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

if [[ -n "${KIND}" ]]; then
	BOARD="${BOARD:-${ZPLATFORM_DEFAULT_BOARD}}"
	zplatform_resolve_app "${KIND}" "${NAME}" "${BOARD}"
	zplatform_resolve_build_dir "${BOARD}" "${APP_KIND}" "${APP_NAME}" "${CONFIG}"
else
	zplatform_read_last_build
	BOARD="${BOARD:-${board}}"
	if [[ -n "${CONFIG}" && "${CONFIG}" != "${configuration}" ]]; then
		zplatform_resolve_build_dir "${BOARD}" "${kind}" "${application}" "${CONFIG}"
	else
		BUILD_DIR="${build_dir}"
		if [[ ! -d "${BUILD_DIR}/zephyr" ]]; then
			echo "error: last build missing at ${BUILD_DIR}" >&2
			exit 1
		fi
	fi
fi

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
zplatform_west debug "${ARGS[@]}"
