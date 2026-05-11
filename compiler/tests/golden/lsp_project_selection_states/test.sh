#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
JZ_HDL="$(python3 -c 'import os,sys; print(os.path.abspath(sys.argv[1]))' "${1:-${SCRIPT_DIR}/../../../build/jz-hdl}")"
TMP_DIR="$(mktemp -d)"
OUT_FILE="$(mktemp)"
ERR_FILE="$(mktemp)"
trap 'rm -rf "${TMP_DIR}" "${OUT_FILE}" "${ERR_FILE}"' EXIT

python3 - "${TMP_DIR}" <<'PY'
import pathlib
import sys

root = pathlib.Path(sys.argv[1])
(root / "project_alpha.jz").write_text(
    "@project PROJECT_ALPHA\n"
    "    @import \"shared/shared_module.jz\";\n"
    "    @top AlphaTop { OUT [1] q = _; }\n"
    "@endproj\n\n"
    "@module AlphaTop\n"
    "    PORT { OUT [1] q; }\n"
    "    ASYNCHRONOUS { q = 1'b0; }\n"
    "@endmod\n"
)
(root / "project_beta.jz").write_text(
    "@project PROJECT_BETA\n"
    "    @import \"shared/shared_module.jz\";\n"
    "    @top BetaTop { OUT [1] q = _; }\n"
    "@endproj\n\n"
    "@module BetaTop\n"
    "    PORT { OUT [1] q; }\n"
    "    ASYNCHRONOUS { q = 1'b1; }\n"
    "@endmod\n"
)
(root / "orphan.jz").write_text(
    "@module OrphanModule\n"
    "    PORT { OUT [1] q; }\n"
    "    ASYNCHRONOUS { q = 1'b0; }\n"
    "@endmod\n"
)
(root / "shared").mkdir()
(root / "shared" / "shared_module.jz").write_text(
    "@module SharedHelper\n"
    "    PORT { OUT [1] q; }\n"
    "    WIRE { local_sig [1]; }\n"
    "    ASYNCHRONOUS {\n"
    "        local_sig = 1'b1;\n"
    "        q = local_sig;\n"
    "    }\n"
    "@endmod\n"
)
PY

python3 - "${TMP_DIR}" <<'PY' | "${JZ_HDL}" --lsp >"${OUT_FILE}" 2>"${ERR_FILE}"
import json
import pathlib
import sys

root = pathlib.Path(sys.argv[1])
orphan_uri = root.joinpath("orphan.jz").as_uri()
shared_uri = root.joinpath("shared", "shared_module.jz").as_uri()
alpha_path = str(root / "project_alpha.jz")

messages = [
    {"jsonrpc": "2.0", "id": 1, "method": "initialize",
     "params": {"rootUri": root.as_uri()}},
    {"jsonrpc": "2.0", "method": "initialized", "params": {}},
    {"jsonrpc": "2.0", "method": "textDocument/didOpen",
     "params": {"textDocument": {
         "uri": orphan_uri,
         "languageId": "jz-hdl",
         "version": 1,
         "text": root.joinpath("orphan.jz").read_text(),
     }}},
    {"jsonrpc": "2.0", "method": "textDocument/didOpen",
     "params": {"textDocument": {
         "uri": shared_uri,
         "languageId": "jz-hdl",
         "version": 1,
         "text": root.joinpath("shared", "shared_module.jz").read_text(),
     }}},
    {"jsonrpc": "2.0", "method": "jz-hdl/selectProject",
     "params": {"uri": shared_uri, "projectFile": alpha_path}},
    {"jsonrpc": "2.0", "id": 2, "method": "shutdown", "params": {}},
    {"jsonrpc": "2.0", "method": "exit", "params": {}},
]

for msg in messages:
    data = json.dumps(msg).encode("utf-8")
    sys.stdout.write(f"Content-Length: {len(data)}\r\n\r\n")
    sys.stdout.flush()
    sys.stdout.buffer.write(data)
    sys.stdout.buffer.flush()
PY

grep -q '"selectionState":"not-imported"' "${OUT_FILE}" || {
    echo "FAIL: LSP did not report the non-imported project state"
    cat "${OUT_FILE}"
    exit 1
}

grep -q 'Project discovery found 2 project(s), but none import this file. Select a project manually or add an @import for this file.' "${OUT_FILE}" || {
    echo "FAIL: LSP did not emit the stable non-imported detail message"
    cat "${OUT_FILE}"
    exit 1
}

grep -q '"selectionState":"ambiguous"' "${OUT_FILE}" || {
    echo "FAIL: LSP did not report ambiguous project selection"
    cat "${OUT_FILE}"
    exit 1
}

grep -q 'Ambiguous project selection: 2 discovered projects import this file. Select the intended project.' "${OUT_FILE}" || {
    echo "FAIL: LSP did not emit the stable ambiguous-project detail message"
    cat "${OUT_FILE}"
    exit 1
}

grep -q '"selectionState":"active"' "${OUT_FILE}" || {
    echo "FAIL: LSP did not report an active project after manual selection"
    cat "${OUT_FILE}"
    exit 1
}

grep -q 'project_alpha.jz' "${OUT_FILE}" || {
    echo "FAIL: LSP did not retain the selected project in projectInfo output"
    cat "${OUT_FILE}"
    exit 1
}

exit 0
