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
(root / "proj.jz").write_text(
    "@project LSP_CACHE\n"
    "    @top Top { OUT [1] q = _; }\n"
    "@endproj\n\n"
    "@module Top\n"
    "    PORT { OUT [1] q; }\n"
    "    ASYNCHRONOUS { q = 1'b0; }\n"
    "@endmod\n"
)
(root / "mod.jz").write_text(
    "@module Helper\n"
    "    PORT { OUT [1] q; }\n"
    "    ASYNCHRONOUS { q = 1'b0; }\n"
    "@endmod\n"
)
PY

python3 - "${TMP_DIR}" <<'PY' | "${JZ_HDL}" --lsp >"${OUT_FILE}" 2>"${ERR_FILE}"
import json
import pathlib
import sys

root = pathlib.Path(sys.argv[1])
module_uri = root.joinpath("mod.jz").as_uri()
messages = [
    {"jsonrpc": "2.0", "id": 1, "method": "initialize",
     "params": {"rootUri": root.as_uri()}},
    {"jsonrpc": "2.0", "method": "initialized", "params": {}},
    {"jsonrpc": "2.0", "method": "textDocument/didOpen",
     "params": {"textDocument": {
         "uri": module_uri,
         "languageId": "jz-hdl",
         "version": 1,
         "text": root.joinpath("mod.jz").read_text(),
     }}},
    {"jsonrpc": "2.0", "method": "textDocument/didOpen",
     "params": {"textDocument": {
         "uri": module_uri,
         "languageId": "jz-hdl",
         "version": 2,
         "text": root.joinpath("mod.jz").read_text(),
     }}},
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

DISCOVERED_COUNT="$(grep -c "discovered " "${ERR_FILE}" || true)"
if [ "${DISCOVERED_COUNT}" -gt 1 ]; then
    echo "FAIL: LSP discovery rescanned the same workspace more than once"
    cat "${ERR_FILE}"
    exit 1
fi

python3 - <<'PY' | "${JZ_HDL}" --lsp >/dev/null 2>"${ERR_FILE}" &
import sys
body = b'{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"rootUri":"file:///tmp"'
sys.stdout.write(f"Content-Length: {len(body)}\r\n\r\n")
sys.stdout.flush()
sys.stdout.buffer.write(body[:20])
sys.stdout.buffer.flush()
PY
PID=$!
sleep 1
if kill -0 "${PID}" 2>/dev/null; then
    kill "${PID}" 2>/dev/null || true
    wait "${PID}" || true
    echo "FAIL: LSP server blocked indefinitely on a partial body"
    cat "${ERR_FILE}"
    exit 1
fi

grep -Eq "timed out waiting for LSP body bytes|unexpected EOF in LSP message body" "${ERR_FILE}" || {
    echo "FAIL: LSP partial-body handling was not reported"
    cat "${ERR_FILE}"
    exit 1
}

exit 0
