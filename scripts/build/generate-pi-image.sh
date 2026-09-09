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

# Check the pinned submodule before spending time compiling.
if [[ ! -f "$PI_GEN_DIR/build-docker.sh" ]]; then
    echo "ERROR: Initialize the pinned image generator with git submodule update --init mixxx-pi-gen" >&2
    exit 1
fi

# Reuse only an intact ARM64 install from the current source and version.
if ! python3 "$REPO_DIR/scripts/build/build-support.py" verify --platform linux/arm64; then
    echo "==> Building fresh, verified ARM64 artifacts..."
    "$REPO_DIR/scripts/build/docker-build.sh" --platform linux/arm64
fi
python3 "$REPO_DIR/scripts/build/build-support.py" verify --platform linux/arm64

# Image naming must agree with the actual binary, including its prerelease.
python3 - "$DIST_DIR/build-provenance.json" "$PI_GEN_DIR/config" <<'VERSION'
import json, pathlib, re, sys
version = json.loads(pathlib.Path(sys.argv[1]).read_text())["version"]
config = pathlib.Path(sys.argv[2]).read_text()
match = re.search(r'^IMG_NAME=["\']?([^"\'\n]+)', config, re.M)
if not match or not match[1].endswith('-v' + version):
    raise SystemExit('Set pi-gen config IMG_NAME to bitedj-pi-v' + version + ' before generating the image')
VERSION

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
