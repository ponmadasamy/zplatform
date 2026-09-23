#!/usr/bin/env bash
# Copyright (c) 2026 ZPlatform contributors
# SPDX-License-Identifier: Apache-2.0
#
# Shared helpers for ZPlatform developer scripts.

set -euo pipefail

ZPLATFORM_DEFAULT_BOARD="${ZPLATFORM_DEFAULT_BOARD:-stm3210e_eval}"

zplatform_root() {
	local here
	# scripts/sh -> repository root
	here="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
	echo "${here}"
}

zplatform_venv_python() {
	local root
	root="$(zplatform_root)"
	if [[ -x "${root}/.venv/bin/python" ]]; then
		echo "${root}/.venv/bin/python"
	elif [[ -x "${root}/.venv/Scripts/python.exe" ]]; then
		echo "${root}/.venv/Scripts/python.exe"
	else
		return 1
	fi
}

zplatform_venv_west() {
	local root
	root="$(zplatform_root)"
	if [[ -x "${root}/.venv/bin/west" ]]; then
		echo "${root}/.venv/bin/west"
	elif [[ -x "${root}/.venv/Scripts/west.exe" ]]; then
		echo "${root}/.venv/Scripts/west.exe"
	else
		return 1
	fi
}

# Load toolchain defaults (board) without Python when possible.
zplatform_load_default_board() {
	local root cfg
	root="$(zplatform_root)"
	cfg="${root}/config/toolchain.yml"
	if [[ -f "${cfg}" ]]; then
		local b
		b="$(grep -E '^\s*default_board:' "${cfg}" | head -n1 | sed 's/.*:[[:space:]]*//' | tr -d '"')"
		if [[ -n "${b}" ]]; then
			ZPLATFORM_DEFAULT_BOARD="${b}"
		fi
	fi
}

zplatform_bootstrap_env() {
	local root
	root="$(zplatform_root)"
	# Always go through setup to create venv / validate pins.
	if [[ ! -x "$(zplatform_venv_python 2>/dev/null || true)" ]]; then
		bash "${root}/scripts/sh/setup-zephyr.sh" --ensure
	fi
}

# Validate environment and export scoped Zephyr variables for this process.
# Does NOT trust a pre-existing global ZEPHYR_BASE.
# Nested scripts (mcuboot_demo → build) inherit ZPLATFORM_ENV_READY and skip.
zplatform_setup_env() {
	local root py env_json
	root="$(zplatform_root)"

	if [[ "${ZPLATFORM_ENV_READY:-}" == "1" && -n "${ZEPHYR_BASE:-}" && -d "${ZEPHYR_BASE}" ]]; then
		return 0
	fi

	zplatform_bootstrap_env
	py="$(zplatform_venv_python)"

	env_json="$("${py}" "${root}/scripts/env/zplatform_env.py" --ensure --print-env)"
	eval "$(echo "${env_json}" | "${py}" -c '
import sys, json
d = json.load(sys.stdin)
keys = [
    "ZEPHYR_BASE", "ZEPHYR_SDK_INSTALL_DIR", "ZEPHYR_TOOLCHAIN_VARIANT",
    "ZPLATFORM_ROOT", "ZPLATFORM_WEST_WORKSPACE", "ZPLATFORM_PYTHON",
    "ZPLATFORM_WEST", "VIRTUAL_ENV",
]
for k in keys:
    v = d.get(k, "")
    # Escape for eval
    v = v.replace("\\", "\\\\").replace("\"", "\\\"")
    print(f"export {k}=\"{v}\"")
prefix = d.get("PATH_PREFIX", "")
prefix = prefix.replace("\\", "\\\\").replace("\"", "\\\"")
print(f"export PATH=\"{prefix}:${{PATH}}\"")
print("export ZPLATFORM_ENV_READY=1")
')"

	if [[ -z "${ZEPHYR_BASE:-}" || ! -d "${ZEPHYR_BASE}" ]]; then
		echo "error: ZEPHYR_BASE not established by environment manager" >&2
		return 1
	fi
}

zplatform_print_build_banner() {
	local root py board app cfg
	root="$(zplatform_root)"
	py="$(zplatform_venv_python)"
	board="$1"
	app="$2"
	cfg="$3"
	"${py}" "${root}/scripts/env/zplatform_env.py" \
		--banner \
		--board "${board}" \
		--application "${app}" \
		--config-name "${cfg}"
}

zplatform_write_fingerprint() {
	local root py board app build_dir
	root="$(zplatform_root)"
	py="$(zplatform_venv_python)"
	board="$1"
	app="$2"
	build_dir="$3"
	"${py}" "${root}/scripts/env/zplatform_env.py" \
		--write-fingerprint "${build_dir}" \
		--board "${board}" \
		--application "${app}"
}

zplatform_require_cmd() {
	local cmd="$1"
	if ! command -v "${cmd}" >/dev/null 2>&1; then
		echo "error: required command not found: ${cmd}" >&2
		return 1
	fi
}

zplatform_west() {
	# Always use project-venv west, never ambient PATH west alone.
	# Run from repo root (same drive as app/build). ZEPHYR_BASE locates the
	# west workspace — avoids Windows cross-drive os.path.relpath failures
	# when the workspace is on C: and this clone is on another drive.
	local w root
	w="$(zplatform_venv_west)"
	root="$(zplatform_root)"
	(
		cd "${root}"
		"${w}" "$@"
	)
}

zplatform_resolve_app() {
	local root kind name board
	root="$(zplatform_root)"
	kind="${1:-}"
	name="${2:-}"
	board="${3:-${BOARD:-${ZPLATFORM_DEFAULT_BOARD}}}"

	case "${kind}" in
	demo)
		APP_KIND="demo"
		APP_NAME="demo"
		APP_PATH="${root}/demo/${board}"
		if [[ ! -d "${APP_PATH}" ]]; then
			echo "error: no demo for board '${board}' (expected ${APP_PATH})" >&2
			return 1
		fi
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
	# Args: board kind name config
	#   demo  -> build/<board>/demo/<config>
	#   sample -> build/<board>/sample/<name>/<config>
	#   boot  -> build/<board>/boot/<name>/<config>
	local root board kind name cfg
	root="$(zplatform_root)"
	board="${1}"
	kind="${2}"
	name="${3:-}"
	cfg="${4:-default}"

	case "${kind}" in
	demo)
		echo "${root}/build/${board}/demo/${cfg}"
		;;
	sample)
		if [[ -z "${name}" ]]; then
			echo "error: sample name required for build dir" >&2
			return 1
		fi
		echo "${root}/build/${board}/sample/${name}/${cfg}"
		;;
	boot)
		name="${name:-mcuboot}"
		echo "${root}/build/${board}/boot/${name}/${cfg}"
		;;
	*)
		echo "error: unknown kind '${kind}' for build dir" >&2
		return 1
		;;
	esac
}

zplatform_app_parent_dir() {
	local root board kind name
	root="$(zplatform_root)"
	board="${1}"
	kind="${2}"
	name="${3:-}"
	case "${kind}" in
	demo) echo "${root}/build/${board}/demo" ;;
	sample) echo "${root}/build/${board}/sample/${name}" ;;
	boot) echo "${root}/build/${board}/boot/${name:-mcuboot}" ;;
	*) echo "error: unknown kind '${kind}'" >&2; return 1 ;;
	esac
}

zplatform_list_configs() {
	local parent
	parent="$(zplatform_app_parent_dir "$@")" || return 1
	if [[ ! -d "${parent}" ]]; then
		return 0
	fi
	local d
	for d in "${parent}"/*/; do
		[[ -d "${d}" ]] || continue
		if [[ -d "${d}zephyr" ]]; then
			basename "${d}"
		fi
	done
}

# Remove build artifacts.
# Usage:
#   zplatform_remove_build --all
#   zplatform_remove_build --board BOARD
#   zplatform_remove_build --board BOARD --kind demo [--name NAME] [--config CFG]
zplatform_remove_build() {
	local all=0 board="" kind="" name="" cfg="" root build_root path parent
	root="$(zplatform_root)"
	build_root="${root}/build"

	while [[ $# -gt 0 ]]; do
		case "$1" in
		--all) all=1; shift ;;
		--board) board="$2"; shift 2 ;;
		--kind) kind="$2"; shift 2 ;;
		--name) name="$2"; shift 2 ;;
		--config) cfg="$2"; shift 2 ;;
		*) echo "error: zplatform_remove_build: unknown arg $1" >&2; return 1 ;;
		esac
	done

	if [[ "${all}" -eq 1 ]]; then
		if [[ -d "${build_root}" ]]; then
			echo "Cleaning all under ${build_root}"
			find "${build_root}" -mindepth 1 -maxdepth 1 -exec rm -rf {} +
		else
			echo "Nothing to clean (${build_root} missing)"
		fi
		return 0
	fi

	if [[ -z "${board}" ]]; then
		echo "error: --board required unless --all" >&2
		return 1
	fi

	if [[ -z "${kind}" ]]; then
		path="${build_root}/${board}"
		if [[ -d "${path}" ]]; then
			echo "Cleaning ${path}"
			rm -rf "${path}"
		fi
		return 0
	fi

	if [[ -n "${cfg}" ]]; then
		path="$(zplatform_build_dir "${board}" "${kind}" "${name}" "${cfg}")"
		if [[ -d "${path}" ]]; then
			echo "Cleaning ${path}"
			rm -rf "${path}"
		fi
		return 0
	fi

	parent="$(zplatform_app_parent_dir "${board}" "${kind}" "${name}")"
	if [[ -d "${parent}" ]]; then
		echo "Cleaning ${parent}"
		rm -rf "${parent}"
	fi
}

# Resolve existing build for flash/debug. Sets BUILD_DIR and CFG.
# Args: board kind name [config]
zplatform_resolve_build_dir() {
	local board kind name cfg root meta mb mk ma mc md
	local -a avail
	local count c

	board="${1}"
	kind="${2}"
	name="${3:-}"
	cfg="${4:-}"

	if [[ -n "${cfg}" ]]; then
		BUILD_DIR="$(zplatform_build_dir "${board}" "${kind}" "${name}" "${cfg}")"
		if [[ ! -d "${BUILD_DIR}/zephyr" ]]; then
			avail=()
			mapfile -t avail < <(zplatform_list_configs "${board}" "${kind}" "${name}")
			echo "error: no build at ${BUILD_DIR} (available: ${avail[*]:-none})" >&2
			return 1
		fi
		CFG="${cfg}"
		return 0
	fi

	root="$(zplatform_root)"
	meta="${root}/build/.zplatform-last-build"
	if [[ -f "${meta}" ]]; then
		mb="$(grep -E '^board=' "${meta}" | head -n1 | cut -d= -f2-)"
		mk="$(grep -E '^kind=' "${meta}" | head -n1 | cut -d= -f2-)"
		ma="$(grep -E '^application=' "${meta}" | head -n1 | cut -d= -f2-)"
		mc="$(grep -E '^configuration=' "${meta}" | head -n1 | cut -d= -f2-)"
		md="$(grep -E '^build_dir=' "${meta}" | head -n1 | cut -d= -f2-)"
		if [[ "${mb}" == "${board}" && "${mk}" == "${kind}" && "${ma}" == "${name}" && -d "${md}/zephyr" ]]; then
			BUILD_DIR="${md}"
			CFG="${mc}"
			return 0
		fi
	fi

	avail=()
	mapfile -t avail < <(zplatform_list_configs "${board}" "${kind}" "${name}")
	count="${#avail[@]}"
	if [[ "${count}" -eq 0 ]]; then
		echo "error: no builds for ${kind}/${name} on ${board} — run build first" >&2
		return 1
	fi
	if [[ "${count}" -eq 1 ]]; then
		CFG="${avail[0]}"
		BUILD_DIR="$(zplatform_build_dir "${board}" "${kind}" "${name}" "${CFG}")"
		return 0
	fi
	for c in "${avail[@]}"; do
		if [[ "${c}" == "default" ]]; then
			CFG="default"
			BUILD_DIR="$(zplatform_build_dir "${board}" "${kind}" "${name}" default)"
			return 0
		fi
	done
	echo "error: multiple configs for ${kind}/${name} on ${board} (${avail[*]}) — pass --config" >&2
	return 1
}

zplatform_write_last_build() {
	# Args: board application build_dir config kind
	local root meta
	root="$(zplatform_root)"
	meta="${root}/build/.zplatform-last-build"
	mkdir -p "$(dirname "${meta}")"
	cat >"${meta}" <<EOF
board=${1}
kind=${5}
application=${2}
configuration=${4}
build_dir=${3}
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

zplatform_load_default_board
