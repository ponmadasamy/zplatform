#!/usr/bin/env bash
# Copyright (c) 2026 ZPlatform contributors
# SPDX-License-Identifier: Apache-2.0
#
# Shared helpers for ZPlatform developer scripts.

set -euo pipefail

ZPLATFORM_DEFAULT_BOARD="${ZPLATFORM_DEFAULT_BOARD:-stm3210e_eval}"

zplatform_root() {
	local here
	here="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
	echo "${here}"
}

zplatform_find_workspace() {
	local root dir
	root="$(zplatform_root)"
	dir="${root}"

	while [[ "${dir}" != "/" && -n "${dir}" ]]; do
		if [[ -d "${dir}/.west" ]]; then
			echo "${dir}"
			return 0
		fi
		dir="$(dirname "${dir}")"
	done

	# Sibling workspace layouts: ../zplatform-ws or parent of zplatform/
	if [[ -d "${root}/../zplatform-ws/.west" ]]; then
		echo "$(cd "${root}/../zplatform-ws" && pwd)"
		return 0
	fi
	if [[ "$(basename "${root}")" == "zplatform" && -d "${root}/../.west" ]]; then
		echo "$(cd "${root}/.." && pwd)"
		return 0
	fi

	return 1
}

zplatform_setup_env() {
	local ws
	if [[ -n "${ZEPHYR_BASE:-}" && -d "${ZEPHYR_BASE}" ]]; then
		return 0
	fi

	if ws="$(zplatform_find_workspace)"; then
		# shellcheck disable=SC1091
		if [[ -f "${ws}/zephyr/zephyr-env.sh" ]]; then
			# shellcheck disable=SC1090
			source "${ws}/zephyr/zephyr-env.sh"
		elif [[ -d "${ws}/zephyr" ]]; then
			export ZEPHYR_BASE="${ws}/zephyr"
		fi
	fi

	if [[ -z "${ZEPHYR_BASE:-}" ]]; then
		# Fall back to a common local install used during development.
		if [[ -d "${HOME}/zephyrproject/zephyr" ]]; then
			export ZEPHYR_BASE="${HOME}/zephyrproject/zephyr"
		elif [[ -d "/c/Users/${USER}/zephyrproject/zephyr" ]]; then
			export ZEPHYR_BASE="/c/Users/${USER}/zephyrproject/zephyr"
		fi
	fi

	if [[ -z "${ZEPHYR_BASE:-}" || ! -d "${ZEPHYR_BASE}" ]]; then
		echo "error: ZEPHYR_BASE not set and Zephyr workspace not found" >&2
		echo "hint: west init a workspace (see docs/architecture/platform.md)" >&2
		return 1
	fi

	if [[ -z "${ZEPHYR_TOOLCHAIN_VARIANT:-}" ]]; then
		export ZEPHYR_TOOLCHAIN_VARIANT=zephyr
	fi
}

zplatform_require_cmd() {
	local cmd="$1"
	if ! command -v "${cmd}" >/dev/null 2>&1; then
		echo "error: required command not found: ${cmd}" >&2
		return 1
	fi
}

zplatform_resolve_app() {
	# Sets: APP_KIND APP_NAME APP_PATH
	local root kind name
	root="$(zplatform_root)"
	kind="${1:-}"
	name="${2:-}"

	case "${kind}" in
	demo)
		APP_KIND="demo"
		APP_NAME="demo"
		APP_PATH="${root}/demo/zplatform_demo"
		;;
	sample)
		if [[ -z "${name}" ]]; then
			echo "error: sample name required (e.g. board_bringup)" >&2
			return 1
		fi
		APP_KIND="sample"
		APP_NAME="${name}"
		APP_PATH="${root}/samples/${name}"
		;;
	test)
		if [[ -z "${name}" ]]; then
			echo "error: test path required" >&2
			return 1
		fi
		APP_KIND="test"
		APP_NAME="${name}"
		APP_PATH="${root}/tests/${name}"
		;;
	*)
		echo "error: unknown application kind '${kind}' (demo|sample|test)" >&2
		return 1
		;;
	esac

	if [[ ! -d "${APP_PATH}" ]]; then
		echo "error: application not found: ${APP_PATH}" >&2
		return 1
	fi
}

zplatform_build_dir() {
	local root board app_name cfg
	root="$(zplatform_root)"
	board="${1}"
	app_name="${2}"
	cfg="${3:-default}"
	echo "${root}/build/${board}/${app_name}/${cfg}"
}

zplatform_write_last_build() {
	local root meta
	root="$(zplatform_root)"
	meta="${root}/build/.zplatform-last-build"
	mkdir -p "$(dirname "${meta}")"
	cat >"${meta}" <<EOF
board=${1}
application=${2}
build_dir=${3}
configuration=${4}
kind=${5}
EOF
}

zplatform_read_last_build() {
	local root meta
	root="$(zplatform_root)"
	meta="${root}/build/.zplatform-last-build"
	if [[ ! -f "${meta}" ]]; then
		echo "error: no previous build metadata at ${meta}" >&2
		return 1
	fi
	# shellcheck disable=SC1090
	source "${meta}"
}

zplatform_extra_modules_arg() {
	local root
	root="$(zplatform_root)"
	echo "-DZEPHYR_EXTRA_MODULES=${root}"
}

zplatform_board_root_arg() {
	local root
	root="$(zplatform_root)"
	echo "-DBOARD_ROOT=${root}"
}
