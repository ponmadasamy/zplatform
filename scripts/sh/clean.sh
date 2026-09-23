#!/usr/bin/env bash
# Copyright (c) 2026 ZPlatform contributors
# SPDX-License-Identifier: Apache-2.0
#
# Clean ZPlatform build outputs.
#
#   ./scripts/sh/clean.sh --all
#   ./scripts/sh/clean.sh --board stm3210e_eval
#   ./scripts/sh/clean.sh demo --board stm3210e_eval --config default
#   ./scripts/sh/clean.sh sample gpio --board stm3210e_eval

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck disable=SC1091
source "${SCRIPT_DIR}/common.sh"

usage() {
	cat <<EOF
Usage:
  $0 --all
  $0 --board BOARD
  $0 demo [--board BOARD] [--config CFG]
  $0 sample <name> [--board BOARD] [--config CFG]
  $0 boot [mcuboot] [--board BOARD] [--config CFG]

Scopes:
  --all                 Delete everything under build/
  --board BOARD         Delete build/<board>/ only
  kind [--config CFG]   Targeted: one config, or all configs for that app

Before-build clean:
  ./scripts/sh/build.sh demo --board BOARD --clean
  ./scripts/sh/demo.sh --board BOARD --clean
  ./scripts/sh/mcuboot_demo.sh --board BOARD --clean
EOF
}

ALL=0
BOARD=""
KIND=""
NAME=""
CFG=""

if [[ $# -lt 1 ]]; then
	usage
	exit 1
fi

case "$1" in
-h|--help)
	usage
	exit 0
	;;
--all)
	ALL=1
	shift
	;;
--board)
	BOARD="$2"
	shift 2
	;;
demo|sample|boot)
	KIND="$1"
	shift
	if [[ "${KIND}" == "sample" ]]; then
		if [[ $# -lt 1 || "$1" == --* ]]; then
			echo "error: sample name required" >&2
			exit 1
		fi
		NAME="$1"
		shift
	elif [[ "${KIND}" == "boot" ]]; then
		if [[ $# -gt 0 && "$1" != --* ]]; then
			NAME="$1"
			shift
		else
			NAME="mcuboot"
		fi
	else
		NAME="demo"
	fi
	;;
*)
	echo "error: unknown argument: $1" >&2
	usage
	exit 1
	;;
esac

while [[ $# -gt 0 ]]; do
	case "$1" in
	--board) BOARD="$2"; shift 2 ;;
	--config) CFG="$2"; shift 2 ;;
	-h|--help) usage; exit 0 ;;
	*) echo "error: unknown option: $1" >&2; usage; exit 1 ;;
	esac
done

if [[ "${ALL}" -eq 1 ]]; then
	if [[ -n "${KIND}" || -n "${BOARD}" || -n "${CFG}" ]]; then
		echo "error: --all cannot be combined with --board / kind / --config" >&2
		exit 1
	fi
	zplatform_remove_build --all
	echo "Cleaned (all)."
	exit 0
fi

if [[ -z "${BOARD}" ]]; then
	BOARD="${ZPLATFORM_DEFAULT_BOARD}"
fi

if [[ -z "${KIND}" ]]; then
	zplatform_remove_build --board "${BOARD}"
	echo "Cleaned board=${BOARD}."
	exit 0
fi

ARGS=(--board "${BOARD}" --kind "${KIND}" --name "${NAME}")
if [[ -n "${CFG}" ]]; then
	ARGS+=(--config "${CFG}")
fi
zplatform_remove_build "${ARGS[@]}"
if [[ -n "${CFG}" ]]; then
	echo "Cleaned board=${BOARD} ${KIND}/${NAME}/${CFG}."
else
	echo "Cleaned board=${BOARD} ${KIND}/${NAME} (all configs)."
fi
