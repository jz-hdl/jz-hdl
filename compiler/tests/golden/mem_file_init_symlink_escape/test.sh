#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
JZ_HDL="$(python3 -c 'import os,sys; print(os.path.abspath(sys.argv[1]))' "${1:-${SCRIPT_DIR}/../../../build/jz-hdl}")"
TMP_DIR="$(mktemp -d)"
OUTSIDE_DIR="$(mktemp -d)"
trap 'rm -rf "${TMP_DIR}" "${OUTSIDE_DIR}"' EXIT

mkdir -p "${TMP_DIR}/data"
printf '01\n02\n' > "${OUTSIDE_DIR}/escape.mem"
ln -s "${OUTSIDE_DIR}/escape.mem" "${TMP_DIR}/data/escape.mem"

python3 - "${TMP_DIR}" <<'PY'
import pathlib
import sys

root = pathlib.Path(sys.argv[1])
(root / "test.jz").write_text(
    "@project MEM_FILE_INIT_SYMLINK_ESCAPE\n"
    "    @top Top { OUT [1] q = _; }\n"
    "@endproj\n\n"
    "@module Top\n"
    "    PORT { OUT [1] q; }\n"
    "    MEM {\n"
    "        rom [8] [2] = @file(\"data/escape.mem\") {\n"
    "            OUT rd ASYNC;\n"
    "        };\n"
    "    }\n"
    "    ASYNCHRONOUS { q = rom.rd[1'b0][0]; }\n"
    "@endmod\n"
)
PY

OUT="$("${JZ_HDL}" --lint "${TMP_DIR}/test.jz" 2>&1 || true)"

grep -q 'PATH_SYMLINK_ESCAPE' <<<"${OUT}" || {
    echo "FAIL: MEM @file() symlink escape was not rejected"
    echo "${OUT}"
    exit 1
}

if grep -Eq 'PATH_OUTSIDE_SANDBOX|PATH_TRAVERSAL_FORBIDDEN|PATH_ABSOLUTE_FORBIDDEN' <<<"${OUT}"; then
    echo "FAIL: MEM @file() symlink escape emitted the wrong path-security diagnostic"
    echo "${OUT}"
    exit 1
fi

exit 0
