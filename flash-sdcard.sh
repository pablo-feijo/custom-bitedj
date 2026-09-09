#!/usr/bin/env bash
set -euo pipefail

TARGET_DISK="/dev/disk7"
RAW_DISK="/dev/rdisk7"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# Follow the pinned image-builder config instead of hard-coding a release.
source "${SCRIPT_DIR}/mixxx-pi-gen/config"
IMAGE_DATE="${BITEDJ_IMAGE_DATE:-$(date +%Y-%m-%d)}"
ZIP_FILE="${SCRIPT_DIR}/mixxx-pi-gen/deploy/image_${IMAGE_DATE}-${IMG_NAME}.zip"
IMG_FILE="${SCRIPT_DIR}/mixxx-pi-gen/deploy/${IMAGE_DATE}-${IMG_NAME}.img"

echo "============================================================"
echo "  BiteDJ SD Card Flasher (macOS Terminal)"
echo "============================================================"

if [ ! -f "${ZIP_FILE}" ]; then
    echo "ERROR: ${ZIP_FILE} not found." >&2
    exit 1
fi

if [ ! -b "${TARGET_DISK}" ]; then
    echo "ERROR: Target device ${TARGET_DISK} not found. Please insert your SD card." >&2
    exit 1
fi

echo "Target Disk: ${TARGET_DISK} (Raw Device: ${RAW_DISK})"
diskutil info "${TARGET_DISK}" | grep -E "Device / Media Name|Disk Size|Content" || true

echo ""
echo "1. Unzipping image file..."
if [ ! -f "${IMG_FILE}" ]; then
    unzip -o "${ZIP_FILE}" -d "${SCRIPT_DIR}/mixxx-pi-gen/deploy/"
fi

echo "2. Unmounting ${TARGET_DISK}..."
diskutil unmountDisk "${TARGET_DISK}"

echo "3. Flashing image to ${RAW_DISK} (requires sudo password)..."
sudo dd if="${IMG_FILE}" of="${RAW_DISK}" bs=4m status=progress conv=fsync

echo "4. Ejecting SD card..."
diskutil eject "${TARGET_DISK}"

echo "============================================================"
echo "  SUCCESS! SD Card flashed and safely ejected."
echo "  You can now insert it into your Raspberry Pi and power on."
echo "============================================================"
