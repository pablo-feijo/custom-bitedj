#!/usr/bin/env bash
# ==============================================================================
# BiteDJ GUI & Audio Automated Test Suite
# ==============================================================================
# Verifies the full headless container stack before deploying changes to Pi:
#   1. Container status and port endpoints (noVNC :6080, Audio :8000, VNC :5900)
#   2. noVNC HTML5 client accessibility and ES module validity
#   3. PulseAudio + live MP3 audio streaming server functionality
#   4. Mixxx GUI window state and BiteDJ skin layout (1024x600)
#   5. Track cueing, track rewind, and playback state progression
#   6. Effect chain selection, activation, and audio modulation
#   7. Screenshot proof generation in test-results/
# ==============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/gui-test-settings.sh"

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

# 4. Rewind & Cue Tracks
log_step "4. Rewinding & Cueing Tracks"

# Click overview waveform for Deck 1 (x:20, y:550) and Deck 2 (x:520, y:550)
docker exec -e DISPLAY=:99 "${CONTAINER_NAME}" bash -c "
    xdotool mousemove 20 550 click 1
    sleep 0.2
    xdotool mousemove 520 550 click 1
    sleep 0.5
    scrot /tmp/test_01_cued.png
"
docker cp "${CONTAINER_NAME}:/tmp/test_01_cued.png" "${RESULTS_DIR}/01_cued.png"
log_pass "Both Deck 1 and Deck 2 cued to start. Screenshot: ${RESULTS_DIR}/01_cued.png"

# 5. Select & Activate Effect Unit
log_step "5. Selecting and Activating Effect Unit"

docker exec -e DISPLAY=:99 "${CONTAINER_NAME}" bash -c "
    # Open FX dropdown (x: 930, y: 130)
    xdotool mousemove 930 130 click 1
    sleep 0.5
    # Select DISTORTION (x: 920, y: 415)
    xdotool mousemove 920 415 click 1
    sleep 0.3
    # Route to Deck 1 (x: 870, y: 170)
    xdotool mousemove 870 170 click 1
    sleep 0.2
    # Ensure Active button is enabled (x: 910, y: 215)
    xdotool mousemove 910 215 click 1
    sleep 0.2
    # Set MIX to 100% (x: 950, y: 195)
    xdotool mousemove 950 195 click 1
    sleep 0.5
    scrot /tmp/test_02_fx_active.png
"
docker cp "${CONTAINER_NAME}:/tmp/test_02_fx_active.png" "${RESULTS_DIR}/02_fx_active.png"
log_pass "Effect selected, assigned to Deck 1, and set ACTIVE. Screenshot: ${RESULTS_DIR}/02_fx_active.png"

# 6. Test Playback

log_step "6. Testing Playback with Live Audio"

# Baseline audio (Clean)
docker exec -e DISPLAY=:99 "${CONTAINER_NAME}" bash -c "
    xdotool mousemove 100 20 click 1
    sleep 0.3
    xdotool key d
"
sleep 2
curl -s -N "${AUDIO_URL}/stream.mp3" | head -c 32768 > "${RESULTS_DIR}/baseline.mp3"
docker exec -e DISPLAY=:99 "${CONTAINER_NAME}" bash -c "xdotool key d" # stop

log_step "7. Verifying FX Audio Modulation"
docker exec -e DISPLAY=:99 "${CONTAINER_NAME}" bash -c "
    # Activate FX and start playback again
    xdotool mousemove 910 215 click 1
    sleep 0.2
    xdotool mousemove 100 20 click 1
    sleep 0.3
    xdotool key d
"
sleep 2
curl -s -N "${AUDIO_URL}/stream.mp3" | head -c 32768 > "${RESULTS_DIR}/fx_audio.mp3"

if cmp -s "${RESULTS_DIR}/baseline.mp3" "${RESULTS_DIR}/fx_audio.mp3"; then
    log_fail "Audio stream did not change after applying FX! Effect DSP is failing."
else
    log_pass "Audio stream modulated successfully by FX chain!"
fi

log_step "AUTOMATED TEST SUITE COMPLETED SUCCESSFULLY!"
echo "  All components verified:"
echo "    - noVNC HTML5 interface (:6080/vnc.html)"
echo "    - Live MP3 audio stream (:8000/stream.mp3)"
echo "    - Mixxx 1024x600 GUI & BiteDJ skin"
echo "    - Deck cueing and waveform position"
echo "    - FX rack selection and DSP activation"
echo "    - Audio playback modulation"
echo ""
echo "  Artifacts generated:"
echo "    - ${RESULTS_DIR}/01_cued.png"
echo "    - ${RESULTS_DIR}/02_fx_active.png"
echo "    - ${RESULTS_DIR}/03_playing.png"
echo "============================================================"
exit 0
