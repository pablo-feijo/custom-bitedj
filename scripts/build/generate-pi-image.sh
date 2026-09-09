#!/usr/bin/env bash
set -euo pipefail

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
DIST_DIR="${REPO_DIR}/dist-linux"
PI_GEN_DIR="${REPO_DIR}/mixxx-pi-gen"

echo "============================================================"
echo "  BiteDJ Raspberry Pi OS Image Builder"
echo "============================================================"

# 1. Verify Docker is available and responsive
if ! docker info >/dev/null 2>&1; then
    echo "ERROR: Docker daemon is not running or accessible." >&2
    echo "Please start Docker Desktop and ensure your user has permission to connect." >&2
    exit 1
fi

# 2. Check if pre-built BiteDJ Linux ARM64 binary exists
if [ ! -f "${DIST_DIR}/bin/mixxx" ] && [ ! -f "${DIST_DIR}/bin/bitedj" ]; then
    echo "==> BiteDJ build artifacts not found in dist-linux/"
    echo "    Compiling BiteDJ Linux ARM64 binary first with ./scripts/build/docker-build.sh..."
    "${REPO_DIR}/scripts/build/docker-build.sh" --platform linux/arm64
fi

if [ ! -f "${DIST_DIR}/bin/mixxx" ] && [ ! -f "${DIST_DIR}/bin/bitedj" ]; then
    echo "ERROR: dist-linux/bin/mixxx was not generated." >&2
    exit 1
fi

echo "==> BiteDJ pre-built artifacts detected in ${DIST_DIR}."

# 3. Ensure mixxx-pi-gen is present
if [ ! -d "${PI_GEN_DIR}" ]; then
    echo "ERROR: ${PI_GEN_DIR} not found." >&2
    exit 1
fi

# 4. Run pi-gen container build
echo "==> Launching Raspberry Pi OS image generation container..."
cd "${PI_GEN_DIR}"
./build-docker.sh "$@"

echo ""
echo "============================================================"
echo "  Raspberry Pi image build completed!"
echo "  Output images and checksums are located in:"
echo "  ${PI_GEN_DIR}/deploy/"
echo "============================================================"
