#!/usr/bin/env bash
set -euo pipefail

TARGET_DISK=""
CHECK_ONLY=""
REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
# Follow the pinned image-builder config instead of hard-coding a release.
source "${REPO_DIR}/mixxx-pi-gen/config"
IMAGE_DATE="${BITEDJ_IMAGE_DATE:-$(date +%Y-%m-%d)}"
ZIP_FILE="${REPO_DIR}/mixxx-pi-gen/deploy/image_${IMAGE_DATE}-${IMG_NAME}.zip"
IMG_FILE="${REPO_DIR}/mixxx-pi-gen/deploy/${IMAGE_DATE}-${IMG_NAME}.img"

echo "============================================================"
echo "  BiteDJ SD Card Flasher (macOS Terminal)"
echo "============================================================"

for ARG in "$@"; do
    case "${ARG}" in
        --check) CHECK_ONLY="--check" ;;
        /dev/disk[0-9]*)
            if [ -n "${TARGET_DISK}" ]; then
                echo "ERROR: Pass only one target disk." >&2
                exit 2
            fi
            TARGET_DISK="${ARG}"
            ;;
        *)
            echo "Usage: $0 [/dev/diskN] [--check]" >&2
            exit 2
            ;;
    esac
done

if [ -z "${TARGET_DISK}" ]; then
    CANDIDATES="$(diskutil list external physical | sed -n 's#^/dev/\(disk[0-9][0-9]*\).*#/dev/\1#p')"
    CANDIDATE_COUNT="$(printf '%s\n' "${CANDIDATES}" | sed '/^$/d' | wc -l | tr -d ' ')"
    if [ "${CANDIDATE_COUNT}" -eq 0 ]; then
        echo "ERROR: No external physical disk detected. Insert the SD card and retry." >&2
        exit 1
    fi
    if [ "${CANDIDATE_COUNT}" -ne 1 ]; then
        echo "ERROR: Multiple external physical disks detected; refusing to guess:" >&2
        printf '  %s\n' ${CANDIDATES} >&2
        echo "Retry with the intended whole disk, for example: $0 /dev/disk5" >&2
        exit 1
    fi
    TARGET_DISK="${CANDIDATES}"
    echo "Automatically detected external disk: ${TARGET_DISK}"
fi

if [[ ! "${TARGET_DISK}" =~ ^/dev/disk[0-9]+$ ]]; then
    echo "ERROR: Target must be a whole disk such as /dev/disk5, not a partition." >&2
    exit 2
fi

RAW_DISK="${TARGET_DISK/\/dev\/disk/\/dev\/rdisk}"

if [ ! -f "${ZIP_FILE}" ]; then
    echo "ERROR: ${ZIP_FILE} not found." >&2
    exit 1
fi

if [ ! -b "${TARGET_DISK}" ]; then
    echo "ERROR: Target device ${TARGET_DISK} not found. Please insert your SD card." >&2
    exit 1
fi

DISK_INFO="$(diskutil info "${TARGET_DISK}")"
if ! grep -Eq '^ *Whole: +Yes$' <<<"${DISK_INFO}" ||
        ! grep -Eq '^ *Device Location: +External$' <<<"${DISK_INFO}" ||
        grep -Eq '^ *Media Read-Only: +Yes$' <<<"${DISK_INFO}"; then
    echo "ERROR: Refusing ${TARGET_DISK}; it must be a writable, whole external disk." >&2
    exit 1
fi

echo "Target Disk: ${TARGET_DISK} (Raw Device: ${RAW_DISK})"
grep -E "Device / Media Name|Disk Size|Content|Device Location|Removable Media" <<<"${DISK_INFO}" || true

if [ "${CHECK_ONLY}" = "--check" ]; then
    echo "READY: Image and detected external SD-card target passed validation."
    exit 0
fi

echo ""
echo "WARNING: This will permanently overwrite all data on ${TARGET_DISK}."
read -r -p "Flash this SD card now? [y/N] " CONFIRM_FLASH
case "${CONFIRM_FLASH}" in
    y|Y|yes|YES|Yes) ;;
    *)
    echo "Cancelled."
    exit 1
    ;;
esac

echo ""
echo "1. Unzipping image file..."
if [ ! -f "${IMG_FILE}" ]; then
    unzip -o "${ZIP_FILE}" -d "${REPO_DIR}/mixxx-pi-gen/deploy/"
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
