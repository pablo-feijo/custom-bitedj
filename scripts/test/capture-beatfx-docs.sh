#!/usr/bin/env bash
# Capture the verified 1024x600 Beat FX picker with two paused synthetic tracks.
set -euo pipefail
REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
source "${REPO_DIR}/scripts/test/gui-test-settings.sh"
verify_test_instance_owner
CAPTURE_DIR="${RESULTS_DIR}/ui-docs"
mkdir -p "${CAPTURE_DIR}"
click() {
    docker exec -e DISPLAY=:99 "${CONTAINER_NAME}" xdotool mousemove "$1" "$2"
    sleep 0.3
    docker exec -e DISPLAY=:99 "${CONTAINER_NAME}" xdotool click 1
    sleep 0.7
}
capture() {
    docker exec -e DISPLAY=:99 "${CONTAINER_NAME}" xdotool mousemove 1023 599
    docker exec -e DISPLAY=:99 "${CONTAINER_NAME}" scrot -o /tmp/beatfx-docs.png
    docker cp "${CONTAINER_NAME}:/tmp/beatfx-docs.png" "${CAPTURE_DIR}/$1.png"
}
# Canonical positions: docs/GUI_TESTING.md, Beat FX picker.
click 100 20
click 878 70
click 934 122
click 564 40  # Standard, page 1
click 762 131 # Echo; standard selection starts Off
capture play
click 934 122
capture beat-fx-page-1
click 918 560
capture beat-fx-page-2
click 692 40
capture beat-fx-saved-1
click 918 560
capture beat-fx-saved-2
click 948 40
{
    git -C "${REPO_DIR}" describe --always --dirty
    shasum -a 256 "${REPO_DIR}/dist-linux/bin/mixxx"
    docker port "${CONTAINER_NAME}"
} > "${CAPTURE_DIR}/beatfx-provenance.txt"
printf 'Review captures before publishing: %s\n' "${CAPTURE_DIR}"
