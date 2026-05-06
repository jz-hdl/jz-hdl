#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
JZ_HDL="$(python3 -c 'import os,sys; print(os.path.abspath(sys.argv[1]))' "${1:-${SCRIPT_DIR}/../../../build/jz-hdl}")"

if [[ ! -x "${JZ_HDL}" ]]; then
    echo "FAIL: jz-hdl binary not found at ${JZ_HDL}"
    exit 1
fi

FAIL=0

check_mode() {
    local mode="$1"
    local ext="$2"
    local expected="${SCRIPT_DIR}/expected.${ext}"
    local actual

    actual="$(mktemp)"
    if ! (cd "${SCRIPT_DIR}" && "${JZ_HDL}" test.jz --verilog --"${mode}" "${actual}" >/dev/null 2>&1); then
        echo "FAIL test.jz (--${mode})"
        rm -f "${actual}"
        FAIL=1
        return
    fi

    if ! diff -u "${expected}" "${actual}" >/dev/null; then
        echo "FAIL test.jz (--${mode})"
        diff -u "${expected}" "${actual}" || true
        FAIL=1
    fi
    rm -f "${actual}"
}

check_mode sdc sdc
check_mode xdc xdc
check_mode pcf pcf
check_mode cst cst

exit "${FAIL}"
