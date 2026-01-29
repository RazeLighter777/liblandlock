#!/usr/bin/env bash
set -euo pipefail

# Download the Linux UAPI Landlock header into this repo so builds don't depend
# on the distro's linux-headers version.
#
# Usage:
#   ./scripts/update-landlock-header.sh            # latest (master)
#   ./scripts/update-landlock-header.sh v6.7      # pin to a tag/branch
#
# Source: torvalds/linux @ include/uapi/linux/landlock.h

REF="${1:-master}"

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_DIR="${ROOT_DIR}/include/linux"
OUT_FILE="${OUT_DIR}/landlock.h"
TMP_FILE="$(mktemp)"

URL="https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/plain/include/uapi/linux/landlock.h?h=${REF}"

mkdir -p "${OUT_DIR}"

echo "Downloading: ${URL}" >&2
curl -fsSL "${URL}" -o "${TMP_FILE}"

# Basic sanity check (avoid committing an HTML error page, etc.).
if ! grep -qE 'Linux-syscall-note|_UAPI_LINUX_LANDLOCK_H|LANDLOCK' "${TMP_FILE}"; then
  echo "Downloaded file does not look like landlock.h (ref=${REF})" >&2
  exit 2
fi

install -m 0644 "${TMP_FILE}" "${OUT_FILE}"
rm -f "${TMP_FILE}"

echo "Wrote: ${OUT_FILE}" >&2
