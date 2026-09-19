#!/usr/bin/env bash
# Copyright (c) 2026 ZPlatform contributors
# SPDX-License-Identifier: Apache-2.0
#
# Build (or rebuild) with profiling configuration and summarize measurements.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck disable=SC1091
source "${SCRIPT_DIR}/common.sh"

usage() {
	cat <<EOF
Usage: $0 [demo|sample <name>] [--board BOARD]

Builds with CONFIG_ZPLATFORM_PROFILING and thread/runtime stats enabled.
Measures (where supported on Cortex-M3):
  - DWT CYCCNT cycle counter samples
  - Zephyr thread runtime statistics (if enabled)
  - Stack high-water marks (CONFIG_INIT_STACKS)

Does not enable Segger SystemView by default (optional, heavy).
EOF
}

KIND="demo"
NAME=""
BOARD="${ZPLATFORM_DEFAULT_BOARD}"

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
zplatform_resolve_app "${KIND}" "${NAME}"

ROOT="$(zplatform_root)"
BUILD_DIR="$(zplatform_build_dir "${BOARD}" "${APP_NAME}" profile)"
CONF_FILE="${ROOT}/demo/zplatform_demo/prj_profile.conf"

if [[ "${APP_KIND}" == "sample" ]]; then
	CONF_FILE=""
fi

mkdir -p "${BUILD_DIR}"

ARGS=(
	-b "${BOARD}"
	-d "${BUILD_DIR}"
	"${APP_PATH}"
	--
	"$(zplatform_board_root_arg)"
	"$(zplatform_extra_modules_arg)"
	-DCONFIG_ZPLATFORM_PROFILING=y
	-DCONFIG_THREAD_RUNTIME_STATS=y
	-DCONFIG_INIT_STACKS=y
	-DCONFIG_THREAD_STACK_INFO=y
	-DCONFIG_THREAD_MONITOR=y
)

if [[ -n "${CONF_FILE}" && -f "${CONF_FILE}" ]]; then
	ARGS+=("-DCONF_FILE=prj.conf;prj_profile.conf")
fi

echo "Executing: west build ${ARGS[*]}"
zplatform_west build "${ARGS[@]}"
zplatform_write_last_build "${BOARD}" "${APP_NAME}" "${BUILD_DIR}" "profile" "${APP_KIND}"
zplatform_write_fingerprint "${BOARD}" "${APP_NAME}" "${BUILD_DIR}"

cat <<EOF

Profiling build ready: ${BUILD_DIR}

What this configuration measures:
  * DWT cycle counter via zplatform_profile_* APIs (Cortex-M3)
  * Thread runtime statistics (CONFIG_THREAD_RUNTIME_STATS)
  * Stack high-water marks (CONFIG_INIT_STACKS)

Flash and run, then use shell 'kernel stacks' / 'kernel threads' or call
zplatform_profile_cycles() around regions of interest.

Hardware validation: NOT PERFORMED by this script alone.
EOF
