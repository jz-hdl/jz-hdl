#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
JZ_HDL="${1:-${SCRIPT_DIR}/../../../build/jz-hdl}"

if [ ! -x "$JZ_HDL" ]; then
    echo "FAIL: jz-hdl binary not found at $JZ_HDL"
    exit 1
fi

TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

OUT_PATH="$TMP_DIR/selects.jzw"

"$JZ_HDL" --simulate "${SCRIPT_DIR}/test.jz" --jzw -o "$OUT_PATH" >/dev/null 2>&1

python3 - "$OUT_PATH" <<'PY'
import sqlite3
import sys

path = sys.argv[1]
db = sqlite3.connect(path)
rows = db.execute(
    """
    SELECT a.type, a.time, a.end_time, a.color, s.scope, s.name
    FROM annotations AS a
    JOIN signals AS s ON s.id = a.signal_id
    WHERE a.type = 'select'
    ORDER BY a.rowid
    """
).fetchall()
db.close()

expected = [
    ("select", 0, 10000, "YELLOW", "wires", "data"),
    ("select", 0, 10000, "CYAN", "dut", "state"),
]

if rows != expected:
    raise SystemExit(f"unexpected select annotations: {rows!r}")
PY

echo "  PASS: JZW emits chained @select range annotations with signal IDs and end_time"
