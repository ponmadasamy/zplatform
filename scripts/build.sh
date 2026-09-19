#!/usr/bin/env bash
# Copyright (c) 2026 ZPlatform contributors
# SPDX-License-Identifier: Apache-2.0
#
# Build a ZPlatform demo or sample.
#
# Usage:
#   ./scripts/build.sh demo [--board BOARD] [--clean] [--debug|--release] [--mcuboot]
#   ./scripts/build.sh sample board_bringup [--board BOARD] ...

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck disable=SC1091
source "${SCRIPT_DIR}/common.sh"

usage() {
	cat <<EOF
Usage:
  $0 demo [options]
  $0 sample <name> [options]

Options:
  --board <name>   Target board (default: ${ZPLATFORM_DEFAULT_BOARD})
  --clean          Remove build directory before configuring
  --debug          Debug-friendly Kconfig (assertions, thread names)
  --release        Optimize for size/speed
  --mcuboot        Build application image for MCUboot slot0
  -h, --help       Show this help
EOF
}

KIND=""
NAME=""
BOARD="${ZPLATFORM_DEFAULT_BOARD}"
CLEAN=0
CFG="default"
MCUBOOT=0
EXTRA_CONF=""

if [[ $# -lt 1 ]]; then
	usage
	exit 1
fi

KIND="$1"
shift

if [[ "${KIND}" == "sample" || "${KIND}" == "test" ]]; then
	if [[ $# -lt 1 || "$1" == --* ]]; then
		echo "error: ${KIND} requires a name" >&2
		exit 1
	fi
	NAME="$1"
	shift
fi

while [[ $# -gt 0 ]]; do
	case "$1" in
	--board)
		BOARD="$2"
		shift 2
		;;
	--clean)
		CLEAN=1
		shift
		;;
	--debug)
		CFG="debug"
		EXTRA_CONF="CONFIG_DEBUG=y CONFIG_DEBUG_OPTIMIZATIONS=y CONFIG_THREAD_NAME=y"
		shift
		;;
	--release)
		CFG="release"
		EXTRA_CONF="CONFIG_SIZE_OPTIMIZATIONS=y"
		shift
		;;
	--mcuboot)
		MCUBOOT=1
		CFG="mcuboot"
		shift
		;;
	-h|--help)
		usage
		exit 0
		;;
	*)
		echo "error: unknown option: $1" >&2
		usage
		exit 1
		;;
	esac
done

zplatform_setup_env
zplatform_require_cmd west
zplatform_resolve_app "${KIND}" "${NAME}"

BUILD_DIR="$(zplatform_build_dir "${BOARD}" "${APP_NAME}" "${CFG}")"
ROOT="$(zplatform_root)"

if [[ "${CLEAN}" -eq 1 && -d "${BUILD_DIR}" ]]; then
	echo "Cleaning ${BUILD_DIR}"
	rm -rf "${BUILD_DIR}"
fi

mkdir -p "${BUILD_DIR}"

CMAKE_ARGS=(
	-b "${BOARD}"
	-d "${BUILD_DIR}"
	"${APP_PATH}"
	--
	"$(zplatform_board_root_arg)"
	"$(zplatform_extra_modules_arg)"
)

OVERLAYS=()
if [[ "${MCUBOOT}" -eq 1 ]]; then
	OVERLAYS+=("${ROOT}/boards/st/stm3210e_eval/stm3210e_eval_mcuboot.overlay")
	CMAKE_ARGS+=("-DCONFIG_BOOTLOADER_MCUBOOT=y")
fi

if [[ ${#OVERLAYS[@]} -gt 0 ]]; then
	OVERLAY_LIST=$(printf "%s;" "${OVERLAYS[@]}")
	OVERLAY_LIST="${OVERLAY_LIST%;}"
	CMAKE_ARGS+=("-DDTC_OVERLAY_FILE=${OVERLAY_LIST}")
fi

if [[ -n "${EXTRA_CONF}" ]]; then
	for kv in ${EXTRA_CONF}; do
		CMAKE_ARGS+=("-D${kv}")
	done
fi

echo "Executing: west build ${CMAKE_ARGS[*]}"
west build "${CMAKE_ARGS[@]}"

zplatform_write_last_build "${BOARD}" "${APP_NAME}" "${BUILD_DIR}" "${CFG}" "${APP_KIND}"

echo
echo "Build complete: ${BUILD_DIR}"
ls -1 "${BUILD_DIR}/zephyr/"zephyr.{elf,bin,hex,map} 2>/dev/null || true
