#!/usr/bin/env bash
# Copyright (c) 2026 ZPlatform contributors
# SPDX-License-Identifier: Apache-2.0
#
# Full MCUboot demo: bootloader + signed app (+ optional notes for flash/upgrade).
#
#   ./scripts/sh/mcuboot_demo.sh --board stm3210e_eval --build
# Hardware flash/upgrade: prefer ./scripts/ps1/mcuboot_demo.ps1 -Flash -Upgrade

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck disable=SC1091
source "${SCRIPT_DIR}/common.sh"

usage() {
	cat <<EOF
Usage:
  $0 [--board BOARD] [--build] [--clean|--clean-all]

  --build (default): MCUboot bootloader + signed slot0 app (v1) + signed v2
  --clean            Before build: remove demo/mcuboot + boot/mcuboot trees
  --clean-all        Before build: remove entire build/<board>/

Hardware flash/upgrade on Windows: scripts/ps1/mcuboot_demo.ps1 -Flash [-Upgrade]

Standalone demo: ./scripts/sh/demo.sh --help
Standalone clean: ./scripts/sh/clean.sh --help
EOF
}

BOARD="${ZPLATFORM_DEFAULT_BOARD}"
DO_BUILD=0
CLEAN=0
CLEAN_ALL=0

while [[ $# -gt 0 ]]; do
	case "$1" in
	--board) BOARD="$2"; shift 2 ;;
	--build) DO_BUILD=1; shift ;;
	--clean) CLEAN=1; shift ;;
	--clean-all) CLEAN_ALL=1; shift ;;
	-h|--help) usage; exit 0 ;;
	*) echo "error: unknown argument: $1" >&2; usage; exit 1 ;;
	esac
done

if [[ "${CLEAN}" -eq 1 && "${CLEAN_ALL}" -eq 1 ]]; then
	echo "error: use only one of --clean or --clean-all" >&2
	exit 1
fi

if [[ "${DO_BUILD}" -eq 0 ]]; then
	DO_BUILD=1
fi

ROOT="$(zplatform_root)"
ROOT_FWD="${ROOT//'\'/'/'}"
KEY="${ROOT}/keys/root-ec-p256.pem"

zplatform_setup_env
zplatform_resolve_app demo demo "${BOARD}"
zplatform_require_cmd west

if [[ "${CLEAN_ALL}" -eq 1 ]]; then
	zplatform_remove_build --board "${BOARD}"
elif [[ "${CLEAN}" -eq 1 ]]; then
	zplatform_remove_build --board "${BOARD}" --kind demo --name demo --config mcuboot
	zplatform_remove_build --board "${BOARD}" --kind boot --name mcuboot --config default
	if [[ -n "${ZPLATFORM_WEST_WORKSPACE:-}" ]]; then
		WS_BOOT="${ZPLATFORM_WEST_WORKSPACE}/build/zplatform/${BOARD}/mcuboot"
		if [[ -d "${WS_BOOT}" ]]; then
			echo "Cleaning ${WS_BOOT}"
			rm -rf "${WS_BOOT}"
		fi
	fi
fi

if [[ ! -f "${KEY}" ]]; then
	echo "error: missing signing key ${KEY}" >&2
	echo "See keys/README.md to generate a development key." >&2
	exit 1
fi

MCUBOOT_CONF="${ROOT}/boards/st/${BOARD}/support/mcuboot.conf"
MCUBOOT_OV="${ROOT}/boards/st/${BOARD}/support/mcuboot.overlay"
if [[ ! -f "${MCUBOOT_CONF}" ]]; then
	echo "error: no MCUboot support for board '${BOARD}' (expected ${MCUBOOT_CONF})" >&2
	exit 1
fi

IMGTOOL=""
if command -v imgtool >/dev/null 2>&1; then
	IMGTOOL="imgtool"
elif [[ -f "${ZEPHYR_BASE}/../bootloader/mcuboot/scripts/imgtool.py" ]]; then
	IMGTOOL="python ${ZEPHYR_BASE}/../bootloader/mcuboot/scripts/imgtool.py"
else
	echo "error: imgtool not found" >&2
	exit 1
fi

echo "=== 1) Build MCUboot (board=${BOARD}) ==="
MCUBOOT_DIR="$(zplatform_build_dir "${BOARD}" boot mcuboot default)"
MCUBOOT_SRC_DIR="$(cd "${ZEPHYR_BASE}/../bootloader/mcuboot/boot/zephyr" && pwd)"
WS="${ZPLATFORM_WEST_WORKSPACE:-}"
if [[ -n "${WS}" ]] && command -v cygpath >/dev/null 2>&1; then
	WS="$(cygpath -u "${WS}")"
fi
# Cross-drive: west refuses relpath between app drive and mcuboot source drive.
if [[ -n "${WS}" ]]; then
	_app_drv="$(printf '%s' "${MCUBOOT_DIR}" | tr '[:upper:]' '[:lower:]' | cut -c1-2)"
	_src_drv="$(printf '%s' "${MCUBOOT_SRC_DIR}" | tr '[:upper:]' '[:lower:]' | cut -c1-2)"
	if [[ "${_app_drv}" != "${_src_drv}" ]]; then
		MCUBOOT_DIR="${WS}/build/zplatform/${BOARD}/mcuboot"
	fi
fi
mkdir -p "${MCUBOOT_DIR}"
WEST_BIN="$(zplatform_venv_west)"
(
	if [[ -n "${WS}" && -d "${WS}" ]]; then
		cd "${WS}"
	else
		cd "$(zplatform_root)"
	fi
	"${WEST_BIN}" build -b "${BOARD}" -d "${MCUBOOT_DIR}" \
		"${MCUBOOT_SRC_DIR}" \
		-- \
		"-DBOARD_ROOT=${ROOT_FWD}" \
		"-DZEPHYR_EXTRA_MODULES=${ROOT_FWD}" \
		"-DZPLATFORM_ROOT=${ROOT_FWD}" \
		"-DCONF_FILE=${ROOT_FWD}/boards/st/${BOARD}/support/mcuboot.conf" \
		"-DDTC_OVERLAY_FILE=${ROOT_FWD}/boards/st/${BOARD}/support/mcuboot.overlay"
)

echo "=== 2) Build + sign application (slot0, v1 via west/imgtool) ==="
BUILD_ARGS=(demo --board "${BOARD}" --mcuboot)
"${SCRIPT_DIR}/build.sh" "${BUILD_ARGS[@]}"
APP_DIR="$(zplatform_build_dir "${BOARD}" demo demo mcuboot)"
BIN="${APP_DIR}/zephyr/zephyr.bin"
SIGNED_V1="${APP_DIR}/zephyr/zephyr.signed.bin"
SIGNED_V2="${APP_DIR}/zephyr/zephyr.signed.v2.bin"
if [[ ! -f "${SIGNED_V1}" ]]; then
	echo "error: expected west-signed image missing: ${SIGNED_V1}" >&2
	exit 1
fi

echo "=== 3) Sign v2.0.0 for slot1 upgrade demo ==="
${IMGTOOL} sign \
	--key "${KEY}" \
	--header-size 0x200 \
	--align 8 \
	--version 2.0.0 \
	--slot-size 0x70000 \
	--pad \
	"${BIN}" "${SIGNED_V2}"

echo "SIGNED_V1=${SIGNED_V1} (from west build)"
echo "SIGNED_V2=${SIGNED_V2}"

echo "=== 4) Flash addresses (F103 dual-bank) ==="
echo "  MCUboot : ${MCUBOOT_DIR}/zephyr/zephyr.bin  -> 0x08000000 (bank1)"
echo "  App v1  : ${SIGNED_V1} -> 0x08080000 (slot0 primary, bank2)"
echo "  App v2  : ${SIGNED_V2} -> 0x08010000 (slot1 secondary, bank1)"
echo
echo "Hardware flash/upgrade:"
echo "  ./scripts/ps1/mcuboot_demo.ps1 -Board ${BOARD} -Flash -Upgrade"
echo
echo "mcuboot_demo.sh done (board=${BOARD})."
