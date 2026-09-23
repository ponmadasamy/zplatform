#!/usr/bin/env bash
# Copyright (c) 2026 ZPlatform contributors
# SPDX-License-Identifier: Apache-2.0
#
# Flash an existing build. Never rebuilds.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck disable=SC1091
source "${SCRIPT_DIR}/common.sh"

usage() {
	cat <<EOF
Usage:
  $0 [demo|sample <name>] [--board BOARD] [--config CFG] [--erase] [--runner NAME]

  No app args: flash the last successful build (build/.zplatform-last-build).
  Flash never rebuilds — it only selects an existing build/<board>/... tree.
EOF
}

KIND=""
NAME=""
BOARD=""
CONFIG=""
ERASE=0
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
ROOT_PY="$(zplatform_venv_python)"
ROOT="$(zplatform_root)"
"${ROOT_PY}" "${ROOT}/scripts/env/zplatform_env.py" --require-flash-tools >/dev/null

CFG=""
if [[ -n "${KIND}" ]]; then
	BOARD="${BOARD:-${ZPLATFORM_DEFAULT_BOARD}}"
	zplatform_resolve_app "${KIND}" "${NAME}" "${BOARD}"
	zplatform_resolve_build_dir "${BOARD}" "${APP_KIND}" "${APP_NAME}" "${CONFIG}"
	APP_DISP="${APP_NAME}"
else
	zplatform_read_last_build
	BOARD="${BOARD:-${board}}"
	if [[ -n "${CONFIG}" && "${CONFIG}" != "${configuration}" ]]; then
		zplatform_resolve_build_dir "${BOARD}" "${kind}" "${application}" "${CONFIG}"
		APP_DISP="${application}"
	else
		BUILD_DIR="${build_dir}"
		CFG="${configuration}"
		APP_DISP="${application}"
		if [[ ! -d "${BUILD_DIR}/zephyr" ]]; then
			echo "error: last build missing at ${BUILD_DIR}" >&2
			exit 1
		fi
	fi
fi

echo "Flashing board=${BOARD} app=${APP_DISP} config=${CFG} dir=${BUILD_DIR}"

FLASH_ARGS=(-d "${BUILD_DIR}")
if [[ "${ERASE}" -eq 1 ]]; then
	FLASH_ARGS+=(--erase)
fi
if [[ -n "${RUNNER}" ]]; then
	FLASH_ARGS+=(-r "${RUNNER}")
fi

echo "Executing: west flash ${FLASH_ARGS[*]}"
zplatform_west flash "${FLASH_ARGS[@]}"
echo "Flash complete."
