#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
JZ_HDL="$(python3 -c 'import os,sys; print(os.path.abspath(sys.argv[1]))' "${1:-${SCRIPT_DIR}/../../../build/jz-hdl}")"
OUT_FILE="$(mktemp)"
ERR_FILE="$(mktemp)"
INIT_FILE="${SCRIPT_DIR}/huge_init.bin"
trap 'rm -f "${OUT_FILE}" "${ERR_FILE}" "${INIT_FILE}"' EXIT

python3 - <<'PY' "${INIT_FILE}"
import pathlib, sys
path = pathlib.Path(sys.argv[1])
path.write_bytes(b"\x00" * 500000)
PY

if (cd "${SCRIPT_DIR}" && "${JZ_HDL}" test.jz --rtlil -o "${OUT_FILE}" >"${ERR_FILE}" 2>&1); then
    echo "FAIL: oversized RTLIL memory init unexpectedly succeeded"
    exit 1
fi

if ! grep -q "exceeds the compiler safety emit-size limit" "${ERR_FILE}"; then
    echo "FAIL: missing RTLIL memory init limit error"
    cat "${ERR_FILE}"
    exit 1
fi

exit 0
