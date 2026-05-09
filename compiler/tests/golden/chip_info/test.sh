#!/usr/bin/env bash

set -euo pipefail

JZ_HDL_BIN="$1"
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TMP_LIST="$(mktemp)"
TMP_DETAIL="$(mktemp)"
trap 'rm -f "${TMP_LIST}" "${TMP_DETAIL}"' EXIT

"${JZ_HDL_BIN}" --chip-info > "${TMP_LIST}"
"${JZ_HDL_BIN}" --chip-info gw1nr-9-qn88-c6-i5 > "${TMP_DETAIL}"

diff -u "${DIR}/expected-list.txt" "${TMP_LIST}"
diff -u "${DIR}/expected-detail.txt" "${TMP_DETAIL}"
