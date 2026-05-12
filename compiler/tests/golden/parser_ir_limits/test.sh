#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
JZ_HDL="$(python3 -c 'import os,sys; print(os.path.abspath(sys.argv[1]))' "${1:-${SCRIPT_DIR}/../../../build/jz-hdl}")"
TMP_DIR="$(mktemp -d)"
trap 'rm -rf "${TMP_DIR}"' EXIT

python3 - "${TMP_DIR}" <<'PY'
import pathlib
import sys

root = pathlib.Path(sys.argv[1])

depth = 530
lhs = "q"
for _ in range(depth):
    lhs = "{" + lhs + "}"

feat_then = "        tmp [1];\n"
for _ in range(depth):
    feat_then = "        @feature 1'b1 == 1'b1\n" + feat_then + "        @endfeat\n"

rhs = "a"
for _ in range(depth):
    rhs = "{" + rhs + "}"

(root / "deep_lvalue.jz").write_text(
    "@project PARSER_DEEP_LVALUE\n"
    "    @top Top { OUT [1] q = _; }\n"
    "@endproj\n\n"
    "@module Top\n"
    "    PORT { OUT [1] q; }\n"
    "    ASYNCHRONOUS {\n"
    f"        {lhs} = 1'b0;\n"
    "    }\n"
    "@endmod\n"
)

(root / "deep_feature.jz").write_text(
    "@project PARSER_DEEP_FEATURE\n"
    "    @top Top { OUT [1] q = _; }\n"
    "@endproj\n\n"
    "@module Top\n"
    "    PORT { OUT [1] q; }\n"
    "    WIRE {\n"
    + feat_then +
    "    }\n"
    "    ASYNCHRONOUS {\n"
    "        q = 1'b0;\n"
    "    }\n"
    "@endmod\n"
)

(root / "ir_expansion.jz").write_text(
    "@project IR_EXPANSION_LIMIT\n"
    "    BUS BIG_BUS {\n"
    "        OUT [1] bit;\n"
    "    }\n"
    "    @top Top { IN [1] a = _; OUT [1] q = _; }\n"
    "@endproj\n\n"
    "@module HugeBus\n"
    "    PORT {\n"
    "        IN [1] a;\n"
    "        BUS BIG_BUS SOURCE [1048577] lanes;\n"
    "        OUT [1] q;\n"
    "    }\n"
    "    ASYNCHRONOUS {\n"
    "        q = a;\n"
    "    }\n"
    "@endmod\n"
    "\n"
    "@module Top\n"
    "    PORT { IN [1] a; OUT [1] q; }\n"
    "    @new u HugeBus { IN [1] a = a; OUT [1] q = q; BUS BIG_BUS SOURCE [1048577] lanes = _; };\n"
    "    ASYNCHRONOUS { }\n"
    "@endmod\n"
)
PY

OUT="$("${JZ_HDL}" --lint "${TMP_DIR}/deep_lvalue.jz" 2>&1 || true)"
grep -q "PARSER_EXPR_DEPTH_LIMIT_EXCEEDED" <<<"${OUT}" || {
    echo "FAIL: deep lvalue did not trip PARSER_EXPR_DEPTH_LIMIT_EXCEEDED"
    echo "${OUT}"
    exit 1
}

OUT="$("${JZ_HDL}" --lint "${TMP_DIR}/deep_feature.jz" 2>&1 || true)"
grep -q "PARSER_STMT_DEPTH_LIMIT_EXCEEDED" <<<"${OUT}" || {
    echo "FAIL: deep feature nesting did not trip PARSER_STMT_DEPTH_LIMIT_EXCEEDED"
    echo "${OUT}"
    exit 1
}

OUT="$("${JZ_HDL}" --lint "${TMP_DIR}/ir_expansion.jz" 2>&1 || true)"
grep -q "IR_EXPANSION_LIMIT_EXCEEDED" <<<"${OUT}" || {
    echo "FAIL: oversized BUS expansion did not trip IR_EXPANSION_LIMIT_EXCEEDED"
    echo "${OUT}"
    exit 1
}

exit 0
