#!/usr/bin/env bash
# Copyright (c) 2026 ZPlatform contributors
# SPDX-License-Identifier: Apache-2.0
#
# Run Ztest / Twister tests for ZPlatform.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck disable=SC1091
source "${SCRIPT_DIR}/common.sh"

usage() {
	cat <<EOF
Usage:
  $0                     # run all ZPlatform tests via twister
  $0 platform/smoke      # focused test path under tests/
  $0 --native            # prefer native_sim / unit-style platforms

Hardware tests are only marked passed when executed on real hardware.
EOF
}

ROOT="$(zplatform_root)"
FOCUS=""
NATIVE=0
BOARD="${ZPLATFORM_DEFAULT_BOARD}"

while [[ $# -gt 0 ]]; do
	case "$1" in
	--native)
		NATIVE=1
		shift
		;;
	--board)
		BOARD="$2"
		shift 2
		;;
	-h|--help)
		usage
		exit 0
		;;
	*)
		FOCUS="$1"
		shift
		;;
	esac
done

zplatform_setup_env

OUTDIR="${ROOT}/build/twister"
mkdir -p "${OUTDIR}"

TEST_ROOT="${ROOT}/tests"
if [[ -n "${FOCUS}" ]]; then
	TEST_ROOT="${ROOT}/tests/${FOCUS}"
fi

if [[ ! -d "${TEST_ROOT}" ]]; then
	echo "error: test path not found: ${TEST_ROOT}" >&2
	exit 1
fi

echo "========================================"
echo "ZPlatform Twister"
echo "========================================"
echo "ZEPHYR_BASE : ${ZEPHYR_BASE}"
echo "Workspace   : ${ZPLATFORM_WEST_WORKSPACE}"
echo "west        : ${ZPLATFORM_WEST}"
echo "========================================"

ARGS=(
	-T "${TEST_ROOT}"
	-O "${OUTDIR}"
	-v
	--board-root "${ROOT}"
	-x="$(zplatform_extra_modules_arg)"
	-x="$(zplatform_board_root_arg)"
)

if [[ "${NATIVE}" -eq 1 ]]; then
	ARGS+=(-p native_sim)
else
	ARGS+=(-p "${BOARD}" --build-only)
	echo "Note: default mode is --build-only (no hardware execution)."
	echo "      Connect hardware and remove --build-only / use twister device testing for HARDWARE VERIFIED."
fi

echo "Executing: west twister ${ARGS[*]}"
zplatform_west twister "${ARGS[@]}"
