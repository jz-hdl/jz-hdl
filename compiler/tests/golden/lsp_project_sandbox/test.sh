#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
JZ_HDL="$(python3 -c 'import os,sys; print(os.path.abspath(sys.argv[1]))' "${1:-${SCRIPT_DIR}/../../../build/jz-hdl}")"
OUT_FILE="$(mktemp)"
ERR_FILE="$(mktemp)"
trap 'rm -f "${OUT_FILE}" "${ERR_FILE}" /tmp/jz_hdl_outside_project.jz' EXIT

printf '%s\n' '@project OUTSIDE_PROJECT' \
              '    @top OutsideTop {' \
              '        OUT [1] q = _;' \
              '    }' \
              '@endproj' \
              '' \
              '@module OutsideTop' \
              '    PORT {' \
              '        OUT [1] q;' \
              '    }' \
              '    ASYNCHRONOUS {' \
              '        q = 1'"'"'b0;' \
              '    }' \
              '@endmod' > /tmp/jz_hdl_outside_project.jz

python3 - <<'PY' | "${JZ_HDL}" --lsp >"${OUT_FILE}" 2>"${ERR_FILE}"
import json
import pathlib
import sys

root = pathlib.Path.cwd()
test_dir = root / "compiler/tests/golden/lsp_project_sandbox"
module_uri = test_dir.joinpath("module.jz").as_uri()
inside_uri = test_dir.joinpath("inside_project.jz").as_uri()
outside_path = "/tmp/jz_hdl_outside_project.jz"

messages = [
    {"jsonrpc": "2.0", "id": 1, "method": "initialize",
     "params": {"rootUri": test_dir.as_uri()}},
    {"jsonrpc": "2.0", "method": "initialized", "params": {}},
    {"jsonrpc": "2.0", "method": "textDocument/didOpen",
     "params": {"textDocument": {
         "uri": module_uri,
         "languageId": "jz-hdl",
         "version": 1,
         "text": (test_dir / "module.jz").read_text(),
     }}},
    {"jsonrpc": "2.0", "method": "jz-hdl/selectProject",
     "params": {"uri": module_uri, "projectFile": outside_path}},
    {"jsonrpc": "2.0", "method": "jz-hdl/selectProject",
     "params": {"uri": module_uri, "projectFile": str(test_dir / "inside_project.jz")}},
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

if ! grep -q 'selectProject rejected outside-sandbox project' "${ERR_FILE}"; then
    echo "FAIL: LSP did not reject the outside-sandbox project selection"
    cat "${ERR_FILE}"
    exit 1
fi

if grep -q '/tmp/jz_hdl_outside_project.jz' "${OUT_FILE}"; then
    echo "FAIL: outside-sandbox project leaked into LSP notifications"
    cat "${OUT_FILE}"
    exit 1
fi

if ! grep -q 'inside_project.jz' "${OUT_FILE}"; then
    echo "FAIL: inside-sandbox project selection was not accepted"
    cat "${OUT_FILE}"
    exit 1
fi

exit 0
