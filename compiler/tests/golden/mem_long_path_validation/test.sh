#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
JZ_HDL="$(python3 -c 'import os,sys; print(os.path.abspath(sys.argv[1]))' "${1:-${SCRIPT_DIR}/../../../build/jz-hdl}")"
BASE="$(mktemp -d)"
trap 'rm -rf "${BASE}"' EXIT

LONG_DIR="${BASE}"
for i in $(seq 1 40); do
    LONG_DIR="${LONG_DIR}/segment_${i}_abcdefghij"
done
mkdir -p "${LONG_DIR}"

python3 - "${LONG_DIR}" <<'PY'
import pathlib
import sys

root = pathlib.Path(sys.argv[1])
(root / "init.mem").write_text("0\n1\n")
(root / "test.jz").write_text(
    "@project LONG_MEM_PATH\n"
    "    @top Top { OUT [1] q = _; }\n"
    "@endproj\n\n"
    "@module Top\n"
    "    PORT { OUT [1] q; }\n"
    "    MEM {\n"
    "        rom [1] [2] = @file(\"init.mem\") {\n"
    "            OUT rd ASYNC;\n"
    "        };\n"
    "    }\n"
    "    ASYNCHRONOUS { q = rom.rd[1'b0]; }\n"
    "@endmod\n"
)
PY

OUT="$("${JZ_HDL}" --lint "${LONG_DIR}/test.jz" 2>&1 || true)"
if grep -q "MEM_INIT_FILE_NOT_FOUND" <<<"${OUT}"; then
    echo "FAIL: long validated MEM path was truncated and reopened incorrectly"
    echo "${OUT}"
    exit 1
fi

if grep -Eq "PATH_OUTSIDE_SANDBOX|PATH_SYMLINK_ESCAPE|PATH_TRAVERSAL_FORBIDDEN|PATH_ABSOLUTE_FORBIDDEN" <<<"${OUT}"; then
    echo "FAIL: long-path MEM init test still hit path validation after truncation"
    echo "${OUT}"
    exit 1
fi

exit 0
