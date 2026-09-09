#!/usr/bin/env bash
# ==============================================================================
# BiteDJ GUI & Audio Capture Smoke
# ==============================================================================
# Checks endpoints and records GUI/audio artifacts for review.
# Requires two paused synthetic tracks, default Deck 1 FX routing, and Night mode.
# Native EffectSlotTest verifies DSP; MP3 byte differences do not prove an effect.
# Captures cover:
#   1. Container status and port endpoints (noVNC :6080, Audio :8000, VNC :5900)
#   2. noVNC HTML5 client accessibility and ES module accessibility
#   3. PulseAudio + live MP3 audio streaming server functionality
#   4. Mixxx GUI window state and BiteDJ skin layout (1024x600)
#   5. Track cueing, track rewind, and playback state progression
#   6. Effect chain selection, activation requests, and audio captures
#   7. Screenshot proof generation in test-results/
# ==============================================================================

set -euo pipefail

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
source "${REPO_DIR}/scripts/test/gui-test-settings.sh"

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
    "${REPO_DIR}/scripts/test/run-gui-test.sh"
    sleep 5
fi

if docker ps --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
    log_pass "Container '${CONTAINER_NAME}' is running."
else
    log_fail "Failed to start container '${CONTAINER_NAME}'."
fi

verify_test_instance_owner
WEB_URL="http://localhost:$(test_host_port 6080)"
AUDIO_URL="http://localhost:$(test_host_port 8000)"

# 2. Verify Web & Audio Endpoints
log_step "2. Verifying Network Endpoints"

# Test noVNC Web UI
HTTP_VNC=$(curl -s -o /dev/null -w "%{http_code}" "${WEB_URL}/vnc.html" || echo "000")
if [ "${HTTP_VNC}" = "200" ]; then
    log_pass "noVNC endpoint responding (HTTP 200 at :6080/vnc.html)"
else
    log_fail "noVNC endpoint failed with status: ${HTTP_VNC}"
fi

# Test noVNC rfb.js module load
HTTP_RFB=$(curl -s -o /dev/null -w "%{http_code}" "${WEB_URL}/core/rfb.js" || echo "000")
if [ "${HTTP_RFB}" = "200" ]; then
    log_pass "noVNC ES module core/rfb.js reachable (HTTP 200)"
else
    log_fail "noVNC rfb.js unreachable with status: ${HTTP_RFB}"
fi

# Test Live Audio UI
HTTP_AUDIO=$(curl -s -o /dev/null -w "%{http_code}" "${AUDIO_URL}/" || echo "000")
if [ "${HTTP_AUDIO}" = "200" ]; then
    log_pass "Live Audio web interface responding (HTTP 200 at :8000/)"
else
    log_fail "Live Audio web interface failed with status: ${HTTP_AUDIO}"
fi

# Test Live MP3 Stream throughput (consume 32KB of stream)
STREAM_BYTES=$( (curl -s -N "${AUDIO_URL}/stream.mp3" | head -c 32768 | wc -c || true) | tr -d '[:space:]')
if [ -n "${STREAM_BYTES}" ] && [ "${STREAM_BYTES}" -ge 30000 ]; then
    log_pass "Live MP3 stream producing audio data (${STREAM_BYTES} bytes verified)"
else
    log_fail "Live MP3 stream failed to deliver audio data (got ${STREAM_BYTES} bytes)"
fi

# 3. Verify GUI Display & Mixxx Process
log_step "3. Verifying GUI Display & Mixxx Window"

WINDOW_INFO=$(docker exec -e DISPLAY=:99 "${CONTAINER_NAME}" xdotool search --name "Mixxx" getwindowgeometry || echo "")
if echo "${WINDOW_INFO}" | grep -q "1024x600"; then
    log_pass "Mixxx window active at 1024x600 resolution"
else
    log_fail "Mixxx window not detected or not at 1024x600 resolution: ${WINDOW_INFO}"
fi

# Canonical 1024x600 targets: docs/GUI_TESTING.md, Beat FX picker.
click() {
    docker exec -e DISPLAY=:99 "${CONTAINER_NAME}" xdotool mousemove "$1" "$2"
    sleep 0.3
    docker exec -e DISPLAY=:99 "${CONTAINER_NAME}" xdotool click 1
    sleep 0.7
}
capture() {
    docker exec -e DISPLAY=:99 "${CONTAINER_NAME}" scrot -o /tmp/fx-smoke.png
    docker cp "${CONTAINER_NAME}:/tmp/fx-smoke.png" "${RESULTS_DIR}/$1.png"
}
record_stream() {
    local status=0
    curl --fail --silent --show-error --max-time 3 "${AUDIO_URL}/stream.mp3" \
        -o "${RESULTS_DIR}/$1.mp3" || status=$?
    # A live stream normally ends only at our timeout (curl 28).
    [[ "$status" == 0 || "$status" == 28 ]] || log_fail "Audio request failed: $status"
    [[ -s "${RESULTS_DIR}/$1.mp3" ]] || log_fail "Audio capture is empty"
}

log_step "4. Cueing paused fixture tracks and selecting ECHO"
click 100 20
click 88 204
click 88 407
capture 01_cued
click 878 70
click 934 122
click 564 40
capture 02_picker
click 762 131 # ECHO, starts Off; default fixture routes FX to Deck 1.
capture 03_fx_off

log_step "5. Recording clean and FX playback for review"
click 31 204 # Deck 1 play.
record_stream baseline
click 934 226 # FX On.
capture 04_fx_active
record_stream fx_audio
click 31 204 # Stop fixture playback.
click 934 226 # FX Off.

log_pass "Endpoints responded; GUI and audio artifacts recorded in ${RESULTS_DIR}."
echo "Review the screenshots and listen to both captures."
echo "This capture smoke does not assert cue position, UI state or DSP from MP3 bytes."
echo "Run EffectSlotTest for native activation, timing and DSP regression checks."
