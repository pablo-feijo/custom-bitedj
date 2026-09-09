#!/usr/bin/env bash
set -euo pipefail
TASK_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
source "$TASK_ROOT/scripts/test/gui-test-settings.sh"
verify_test_instance_owner
mkdir -p "$RESULTS_DIR"
# Deliberately no hardware passthrough or host-visible MIDI listener.
docker run --rm --platform linux/arm64 -v "$TASK_ROOT:/src" bitedj-builder-linux-arm64 \
  cc -shared -fPIC -O2 /src/tests/padfx/virtual_portmidi.c \
  -o "/src/test-results/$CONTAINER_NAME/virtual_portmidi.so"
docker cp "$RESULTS_DIR/virtual_portmidi.so" "$CONTAINER_NAME:/tmp/virtual_portmidi.so"
docker exec "$CONTAINER_NAME" pkill -9 mixxx || true
cp "$CONFIG_DIR/mixxx.cfg" "$RESULTS_DIR/pre-midi-test.cfg"
restore() {
  verify_test_instance_owner || return
  docker exec "$CONTAINER_NAME" pkill -9 mixxx || true
  cp "$RESULTS_DIR/pre-midi-test.cfg" "$CONFIG_DIR/mixxx.cfg"
  docker cp "$CONTAINER_NAME:/tmp/pad-midi-live.log" "$RESULTS_DIR/pad-midi-live.log" || true
  docker cp "$CONTAINER_NAME:/tmp/padfx-audio" "$RESULTS_DIR/" || true
  docker exec -d "$CONTAINER_NAME" bash -c 'DISPLAY=:99 QT_AUTO_SCREEN_SCALE_FACTOR=0 QT_ENABLE_HIGHDPI_SCALING=0 QT_SCALE_FACTOR=1.0 BITEDJ_SETTINGS_PATH=/root/.mixxx /dist-linux/bin/mixxx /music/BiteDJ_Test_Groove_128BPM.wav /music/BiteDJ_Test_Techno_124BPM.wav --resourcePath /dist-linux/share/mixxx/ --full-screen --style Fusion'
}
trap restore EXIT
python3 "$TASK_ROOT/tests/padfx/prepare_live_fixture.py" "$TASK_ROOT" "$CONFIG_DIR"
docker exec -d "$CONTAINER_NAME" bash -c 'DISPLAY=:99 QT_AUTO_SCREEN_SCALE_FACTOR=0 QT_ENABLE_HIGHDPI_SCALING=0 QT_SCALE_FACTOR=1.0 BITEDJ_SETTINGS_PATH=/root/.mixxx LD_PRELOAD=/tmp/virtual_portmidi.so /dist-linux/bin/mixxx /music/PadFX_Calibration.wav --resourcePath /dist-linux/share/mixxx/ --full-screen --style Fusion --controllerDebug > /tmp/pad-midi-live.log 2>&1'
# Wait for controller and audio startup; do not send MIDI to a half-loaded deck.
ready=0
for attempt in $(seq 1 45); do
  if docker exec "$CONTAINER_NAME" sh -c 'grep -q "Loading.*piflex-padfx.js" /tmp/pad-midi-live.log && grep -q "slotTrackLoaded.*PadFX_Calibration" /tmp/pad-midi-live.log && grep -q "Loaded skin" /tmp/pad-midi-live.log'; then ready=1; break; fi
  sleep 1
done
if [[ "$ready" != 1 ]]; then echo "Timed out waiting for test controller/track/skin" >&2; exit 1; fi
for script in capture_midi_audio.py capture_tail_audio.py; do
  docker cp "$TASK_ROOT/tests/padfx/$script" "$CONTAINER_NAME:/tmp/$script"
  docker exec "$CONTAINER_NAME" python3 "/tmp/$script"
done
printf 'Audio and MIDI evidence: %s/padfx-audio\n' "$RESULTS_DIR"
