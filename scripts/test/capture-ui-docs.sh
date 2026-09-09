#!/usr/bin/env bash
# Capture the prepared UI; review raw images before promoting them into docs.
set -euo pipefail
REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
source "${REPO_DIR}/scripts/test/gui-test-settings.sh"
verify_test_instance_owner
CAPTURE_DIR="${RESULTS_DIR}/ui-docs"
mkdir -p "${CAPTURE_DIR}"

click() {
    docker exec -e DISPLAY=:99 "${CONTAINER_NAME}" xdotool mousemove "$1" "$2"
    sleep 1
    docker exec -e DISPLAY=:99 "${CONTAINER_NAME}" xdotool click 1
    sleep 1
}

capture() {
    # Move off controls to avoid hover/tooltip state; scrot excludes the cursor.
    docker exec -e DISPLAY=:99 "${CONTAINER_NAME}" xdotool mousemove 1023 599
    sleep 2
    docker exec -e DISPLAY=:99 "${CONTAINER_NAME}" scrot -o /tmp/ui-docs.png
    docker cp "${CONTAINER_NAME}:/tmp/ui-docs.png" "${CAPTURE_DIR}/$1.png"
}

# Requires two loaded, paused synthetic tracks and a prepared Browse table.
# Canonical positions: docs/GUI_TESTING.md, Exact 1024x600 Coordinate Grid.
click 100 20
capture play
click 300 20
capture browse-preview
click 950 20
for page in general:73 library:219 pad-fx:366 device:512 audio:658 system:805 info:951; do
    click "${page##*:}" 60
    capture "settings-${page%%:*}"
done
click 100 20
{
    git -C "${REPO_DIR}" rev-parse HEAD
    git -C "${REPO_DIR}" branch --show-current
    shasum -a 256 "${REPO_DIR}/dist-linux/bin/mixxx"
    docker port "${CONTAINER_NAME}"
} > "${CAPTURE_DIR}/provenance.txt"
printf 'Raw screenshots: %s\nReview each image before copying selected PNGs to docs/images/ui/<version>/.\n' "${CAPTURE_DIR}"
