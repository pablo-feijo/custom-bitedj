#!/usr/bin/env bash
# ==============================================================================
# BiteDJ GUI Automated Test Suite - Library Preview
# ==============================================================================

set -euo pipefail

CONTAINER_NAME="bitedj-gui-test-instance"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RESULTS_DIR="${SCRIPT_DIR}/test-results"

mkdir -p "${RESULTS_DIR}"

log_step() {
    echo ""
    echo "============================================================"
    echo "  $1"
    echo "============================================================"
}

log_pass() {
    echo "  [PASS] $1"
}

log_fail() {
    echo "  [FAIL] $1"
    exit 1
}

# 1. Ensure container is running
log_step "1. Checking BiteDJ Test Container Status"
if ! docker ps --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
    echo "Container '${CONTAINER_NAME}' is not running. Launching via run-gui-test.sh..."
    "${SCRIPT_DIR}/run-gui-test.sh"
    sleep 5
fi

# 2. Test Preview
log_step "2. Testing Library Preview & Analysis"

docker exec -e DISPLAY=:99 "${CONTAINER_NAME}" bash -c "
    # Go to Browse Tab (x=300, y=30)
    xdotool mousemove 300 30 click 1
    sleep 0.5
    
    # Click Music sidebar item (x=100, y=142)
    xdotool mousemove 100 142 click 1
    sleep 1.0

    # Click first track row to trigger analysis (y=218)
    xdotool mousemove 300 218 click 1
    sleep 0.5
    
    # Click second track row (y=240)
    xdotool mousemove 300 240 click 1
    
    # Wait for the background analyzer to fully complete (can take 10+ seconds for a full track)
    sleep 15.0
    
    scrot -o /tmp/test_preview.png

    # Test Drag and Drop label position
    # Move to first track
    xdotool mousemove 300 218
    sleep 0.2
    # Click and hold (mousedown)
    xdotool mousedown 1
    sleep 0.2
    # Drag into the center of the screen
    xdotool mousemove 512 300
    sleep 0.5
    # Capture the drag label hovering over the finger/cursor
    scrot -o /tmp/test_drag.png
    # Release the drag
    xdotool mouseup 1

"
docker cp "${CONTAINER_NAME}:/tmp/test_preview.png" "${RESULTS_DIR}/test_preview.png"
docker cp "${CONTAINER_NAME}:/tmp/test_drag.png" "${RESULTS_DIR}/test_drag.png"
log_pass "Preview tested. Screenshot: ${RESULTS_DIR}/test_preview.png"
log_pass "Drag tested. Screenshot: ${RESULTS_DIR}/test_drag.png"

log_step "AUTOMATED TEST SUITE COMPLETED SUCCESSFULLY!"
exit 0
