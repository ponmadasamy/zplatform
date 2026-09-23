#!/usr/bin/env bash
# Copyright (c) 2026 ZPlatform contributors
# SPDX-License-Identifier: Apache-2.0
#
# ZPlatform Zephyr environment manager (Linux / macOS / Git Bash).

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
ENV_PY="${SCRIPT_DIR}/../env/zplatform_env.py"

find_python() {
	local candidates=("python3.12" "python3.13" "python3.14" "python3" "python")
	local c ver
	for c in "${candidates[@]}"; do
		if command -v "${c}" >/dev/null 2>&1; then
			ver="$("${c}" -c 'import sys; print("%d.%d"%sys.version_info[:2])' 2>/dev/null || true)"
			if [[ -n "${ver}" ]]; then
				# shellcheck disable=SC2072
				if [[ "$(printf '%s\n' "3.12" "${ver}" | sort -V | head -n1)" == "3.12" ]]; then
					# ver >= 3.12
					if [[ "${ver}" == "3.12" || "${ver}" > "3.12" || "${ver}" =~ ^3\.(1[2-9]|[2-9][0-9]) ]]; then
						echo "${c}"
						return 0
					fi
				fi
			fi
		fi
	done
	return 1
}

bootstrap_venv() {
	local py
	if [[ -x "${ROOT}/.venv/bin/python" ]]; then
		return 0
	fi
	if [[ -x "${ROOT}/.venv/Scripts/python.exe" ]]; then
		return 0
	fi
	if ! py="$(find_python)"; then
		echo "ERROR: Python >= 3.12 required to bootstrap ZPlatform environment." >&2
		echo "Install Python 3.12+ then re-run this script." >&2
		exit 1
	fi
	echo "Bootstrapping .venv with ${py}"
	"${py}" -m venv "${ROOT}/.venv"
	# Pip/requirements: single path via zplatform_env.py --ensure (hash-stamped).
}

venv_python() {
	if [[ -x "${ROOT}/.venv/bin/python" ]]; then
		echo "${ROOT}/.venv/bin/python"
	elif [[ -x "${ROOT}/.venv/Scripts/python.exe" ]]; then
		echo "${ROOT}/.venv/Scripts/python.exe"
	else
		echo "ERROR: .venv python missing" >&2
		exit 1
	fi
}

usage() {
	cat <<EOF
Usage: $0 [options]

Options:
  --status           Show environment status
  --ensure           Create/validate environment (default if no args)
  --sync             west update to pinned Zephyr revision
  --sync-manifest    Copy toolchain.yml revision into west.yml
  --check-latest     Query official Zephyr releases (no pin change)
  --update           Same as --check-latest (explicit, non-mutating)
  --help             Show this help
EOF
}

ARGS=("$@")
if [[ $# -eq 0 ]]; then
	ARGS=(--ensure --status)
fi

for a in "${ARGS[@]}"; do
	case "${a}" in
	-h|--help)
		usage
		exit 0
		;;
	esac
done

bootstrap_venv
PY="$(venv_python)"
exec "${PY}" "${ENV_PY}" "${ARGS[@]}"
