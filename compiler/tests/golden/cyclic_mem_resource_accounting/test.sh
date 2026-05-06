#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
JZ_HDL="$(python3 -c 'import os,sys; print(os.path.abspath(sys.argv[1]))' "${1:-${SCRIPT_DIR}/../../../build/jz-hdl}")"
OUT_FILE="$(mktemp)"
trap 'rm -f "${OUT_FILE}"' EXIT

if ! "${JZ_HDL}" --lint "${SCRIPT_DIR}/test.jz" >"${OUT_FILE}" 2>&1; then
    if grep -Eqi 'segmentation fault|stack overflow|abort|bus error' "${OUT_FILE}"; then
        echo "FAIL: cyclic instantiation crashed the compiler"
        cat "${OUT_FILE}"
        exit 1
    fi
fi

exit 0
