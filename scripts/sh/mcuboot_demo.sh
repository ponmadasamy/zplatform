#!/usr/bin/env bash
# Copyright (c) 2026 ZPlatform contributors
# SPDX-License-Identifier: Apache-2.0
#
# Demonstrate MCUboot signed upgrade workflow (no OTA networking).
#
# Prerequisites:
#   - west workspace with mcuboot
#   - development key at keys/root-rsa-2048.pem
#   - ST-Link connected for flash steps

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck disable=SC1091
source "${SCRIPT_DIR}/common.sh"

ROOT="$(zplatform_root)"
BOARD="${ZPLATFORM_DEFAULT_BOARD}"
KEY="${ROOT}/keys/root-rsa-2048.pem"

zplatform_setup_env
zplatform_require_cmd west

if [[ ! -f "${KEY}" ]]; then
	echo "error: missing signing key ${KEY}" >&2
	echo "See keys/README.md to generate a development key." >&2
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

echo "=== 1) Build MCUboot ==="
MCUBOOT_DIR="$(zplatform_build_dir "${BOARD}" mcuboot default)"
mkdir -p "${MCUBOOT_DIR}"
west build -b "${BOARD}" -d "${MCUBOOT_DIR}" \
	"${ZEPHYR_BASE}/../bootloader/mcuboot/boot/zephyr" \
	-- \
	"$(zplatform_board_root_arg)" \
	"$(zplatform_extra_modules_arg)" \
	-DCONF_FILE="${ROOT}/boards/st/stm3210e_eval/support/mcuboot.conf" \
	-DDTC_OVERLAY_FILE="${ROOT}/boards/st/stm3210e_eval/support/mcuboot.overlay"

echo "=== 2) Build signed application v1 ==="
# Public entry: ./scripts/sh/build.sh
"${SCRIPT_DIR}/build.sh" demo --board "${BOARD}" --mcuboot
APP_DIR="$(zplatform_build_dir "${BOARD}" demo mcuboot)"
BIN_V1="${APP_DIR}/zephyr/zephyr.bin"
SIGNED_V1="${APP_DIR}/zephyr/zephyr.signed.v1.bin"

${IMGTOOL} sign \
	--key "${KEY}" \
	--header-size 0x200 \
	--align 8 \
	--version 1.0.0 \
	--slot-size 0x74000 \
	"${BIN_V1}" "${SIGNED_V1}"

echo "=== 3) Flash MCUboot + v1 (requires hardware) ==="
echo "Hardware steps (run when ST-Link is connected):"
echo "  west flash -d ${MCUBOOT_DIR}"
echo "  west flash -d ${APP_DIR}   # or flash ${SIGNED_V1} to slot0"
echo
echo "=== 4) Build signed application v2 ==="
# Rebuild with a different version stamp via conf fragment if desired.
BIN_V2="${APP_DIR}/zephyr/zephyr.bin"
SIGNED_V2="${APP_DIR}/zephyr/zephyr.signed.v2.bin"
${IMGTOOL} sign \
	--key "${KEY}" \
	--header-size 0x200 \
	--align 8 \
	--version 2.0.0 \
	--slot-size 0x74000 \
	"${BIN_V2}" "${SIGNED_V2}"

echo "Place ${SIGNED_V2} into slot1 (0x08084000), reset, and confirm upgrade."
echo
echo "MCUboot workflow prepared (BUILD side). Hardware upgrade: NOT PERFORMED."
