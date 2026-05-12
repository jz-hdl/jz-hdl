#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
JZ_HDL="$(python3 -c 'import os,sys; print(os.path.abspath(sys.argv[1]))' "${1:-${SCRIPT_DIR}/../../../build/jz-hdl}")"

if [[ ! -x "${JZ_HDL}" ]]; then
    echo "FAIL: jz-hdl binary not found at ${JZ_HDL}"
    exit 1
fi

actual="$(mktemp)"
trap 'rm -f "${actual}"' EXIT

if ! (cd "${SCRIPT_DIR}" && "${JZ_HDL}" test.jz --verilog --xdc "${actual}" >/dev/null 2>&1); then
    echo "FAIL test.jz (--xdc)"
    exit 1
fi

if ! diff -u "${SCRIPT_DIR}/expected.xdc" "${actual}" >/dev/null; then
    echo "FAIL test.jz (--xdc)"
    diff -u "${SCRIPT_DIR}/expected.xdc" "${actual}" || true
    exit 1
fi
