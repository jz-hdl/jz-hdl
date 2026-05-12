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

EXPECTED_INIT_HEX="0123456789abcdef0000000000000001ffffffffffffffffffffffffffffffff"
EXPECTED_NEXT_HEX="0123456789abcdef000000000000000200000000000000000000000000000000"

echo "Testing VCD/FST/JZW wide waveform output..."

run_and_check() {
    local format="$1"
    local path="$TMP_DIR/wide.${format}"
    "$JZ_HDL" --simulate "${SCRIPT_DIR}/test.jz" "--${format}" -o "${path}" >/dev/null 2>&1
    python3 - "$format" "$path" "$EXPECTED_INIT_HEX" "$EXPECTED_NEXT_HEX" <<'PY'
import binascii
import pathlib
import sqlite3
import sys

fmt, path, init_hex, next_hex = sys.argv[1:5]
payload = pathlib.Path(path).read_bytes()
init_bits = bin(int(init_hex, 16))[2:].zfill(256)
next_bits = bin(int(next_hex, 16))[2:].zfill(256)

if fmt == "vcd":
    text = payload.decode("utf-8")
    if f"b{init_bits} " not in text:
        raise SystemExit("missing initial 256-bit VCD value")
    if f"b{next_bits} " not in text:
        raise SystemExit("missing incremented 256-bit VCD value")
elif fmt == "jzw":
    db = sqlite3.connect(path)
    rows = db.execute(
        "SELECT time, value FROM changes "
        "JOIN signals ON signals.id = changes.signal_id "
        "WHERE signals.name = 'count' ORDER BY time"
    ).fetchall()
    db.close()
    expected = [(5000, init_bits), (15000, next_bits)]
    filtered = [(time, value) for (time, value) in rows if time in (5000, 15000)]
    if filtered != expected:
        raise SystemExit(f"unexpected JZW rows: {filtered!r}")
elif fmt == "fst":
    init_bytes = binascii.unhexlify(init_hex)
    next_bytes = binascii.unhexlify(next_hex)
    needle = b"\x00" + init_bytes + b"\x02" + next_bytes
    if needle not in payload:
        raise SystemExit("missing expected 256-bit FST VC payload")
else:
    raise SystemExit(f"unknown format {fmt}")
PY
}

run_and_check vcd
echo "  PASS: VCD captures full 256-bit values"

run_and_check fst
echo "  PASS: FST captures full 256-bit values"

run_and_check jzw
echo "  PASS: JZW captures full 256-bit values"
