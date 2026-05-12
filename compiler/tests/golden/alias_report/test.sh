#!/usr/bin/env bash

set -euo pipefail

JZ_HDL_BIN="$1"
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TMP_OUT="$(mktemp)"
trap 'rm -f "${TMP_OUT}"' EXIT

(cd "${DIR}" && "${JZ_HDL_BIN}" fixture.hdl --alias-report > "${TMP_OUT}")
diff -u "${DIR}/expected.txt" "${TMP_OUT}"
