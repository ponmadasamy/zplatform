#!/usr/bin/env bash
# Copyright (c) 2026 ZPlatform contributors
# SPDX-License-Identifier: Apache-2.0
#
# Build a ZPlatform demo or sample.
#
# Usage:
#   ./scripts/sh/build.sh demo [--board BOARD] [--clean] [--debug|--release] [--mcuboot]
#   ./scripts/sh/build.sh sample board_bringup [--board BOARD] ...

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

case "$1" in
-h|--help)
	usage
	exit 0
	;;
esac

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
zplatform_resolve_app "${KIND}" "${NAME}" "${BOARD}"

BUILD_DIR="$(zplatform_build_dir "${BOARD}" "${APP_KIND}" "${APP_NAME}" "${CFG}")"
ROOT="$(zplatform_root)"

zplatform_print_build_banner "${BOARD}" "${APP_NAME}" "${CFG}"

if [[ "${CLEAN}" -eq 1 ]]; then
	zplatform_remove_build --board "${BOARD}" --kind "${APP_KIND}" --name "${APP_NAME}" --config "${CFG}"
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
	OVERLAY="${ROOT}/boards/st/${BOARD}/${BOARD}_mcuboot.overlay"
	if [[ ! -f "${OVERLAY}" ]]; then
		echo "error: MCUboot overlay not found for board ${BOARD} (expected ${OVERLAY})" >&2
		exit 1
	fi
	OVERLAYS+=("${OVERLAY}")
	CMAKE_ARGS+=("-DZPLATFORM_ROOT=${ROOT}")
	CMAKE_ARGS+=("-DCONFIG_BOOTLOADER_MCUBOOT=y")
	CMAKE_ARGS+=("-DCONFIG_STREAM_FLASH=y")
	CMAKE_ARGS+=("-DCONFIG_IMG_MANAGER=y")
	CMAKE_ARGS+=("-DCONFIG_MCUBOOT_IMG_MANAGER=y")
	if [[ ! -f "${ROOT}/keys/root-ec-p256.pem" ]]; then
		echo "error: missing ${ROOT}/keys/root-ec-p256.pem — see keys/README.md" >&2
		exit 1
	fi
	if [[ -f "${APP_PATH}/prj_mcuboot.conf" ]]; then
		CMAKE_ARGS+=("-DEXTRA_CONF_FILE=${APP_PATH}/prj_mcuboot.conf")
	fi
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
zplatform_west build "${CMAKE_ARGS[@]}"

zplatform_write_last_build "${BOARD}" "${APP_NAME}" "${BUILD_DIR}" "${CFG}" "${APP_KIND}"
zplatform_write_fingerprint "${BOARD}" "${APP_NAME}" "${BUILD_DIR}"
"$(zplatform_venv_python)" "$(zplatform_root)/scripts/env/zplatform_env.py" --print-layout "${BUILD_DIR}" || true

echo
echo "Build complete: ${BUILD_DIR}"
ls -1 "${BUILD_DIR}/zephyr/"zephyr.{elf,bin,hex,map} 2>/dev/null || true
ls -1 "${BUILD_DIR}/zplatform-environment".{txt,json} 2>/dev/null || true
