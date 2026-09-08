#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
DIST_DIR="${SCRIPT_DIR}/dist-linux"
MUSIC_DIR="${SCRIPT_DIR}/test-music"
CONFIG_DIR="${SCRIPT_DIR}/test-config"

if [ ! -f "${DIST_DIR}/bin/mixxx" ]; then
    echo "Error: dist-linux/bin/mixxx not found!"
    echo "Please build the ARM64 binary first with: ./docker-build.sh --platform linux/arm64"
    exit 1
fi

echo "==> 1. Synchronizing effect chains and resources to dist-linux..."
mkdir -p "${DIST_DIR}/share/mixxx/effects/chains"
cp -r "${SCRIPT_DIR}/res/effects/chains/"*.xml "${DIST_DIR}/share/mixxx/effects/chains/"
cp -r "${SCRIPT_DIR}/res/skins/BiteDJ" "${DIST_DIR}/share/mixxx/skins/"

echo "==> 2. Ensuring test music tracks exist..."
if [ ! -f "${MUSIC_DIR}/BiteDJ_Test_Groove_128BPM.wav" ]; then
    python3 "${SCRIPT_DIR}/generate_test_music.py"
fi

echo "==> 3. Building/verifying BiteDJ GUI test container..."
docker build -t bitedj-gui-test:latest -f "${SCRIPT_DIR}/tests/gui/Dockerfile.gui-test" "${SCRIPT_DIR}"

echo "==> 4. Launching BiteDJ GUI test instance (1024x600, VNC + PulseAudio)..."
docker rm -f bitedj-gui-test-instance 2>/dev/null || true

mkdir -p "${CONFIG_DIR}"

docker run -d \
    --name bitedj-gui-test-instance \
    -p 6080:6080 \
    -p 8000:8000 \
    -p 5900:5900 \
    -v "${DIST_DIR}:/dist-linux:ro" \
    -v "${MUSIC_DIR}:/music:ro" \
    -v "${CONFIG_DIR}:/root/.mixxx:rw" \
    -v "${SCRIPT_DIR}/audio_stream.py:/audio_stream.py:ro" \
    bitedj-gui-test:latest \
    bash -c "\
        pulseaudio -D --exit-idle-time=-1 --system=false && \
        python3 /audio_stream.py >/dev/null 2>&1 & \
        Xvfb :99 -screen 0 1024x600x24 & \
        sleep 1 && \
        DISPLAY=:99 openbox & \
        x11vnc -display :99 -forever -shared -nopw -bg & \
        websockify --web /usr/share/novnc 6080 localhost:5900 & \
        sleep 1 && \
        DISPLAY=:99 QT_AUTO_SCREEN_SCALE_FACTOR=0 QT_ENABLE_HIGHDPI_SCALING=0 QT_SCALE_FACTOR=1.0 BITEDJ_SETTINGS_PATH=/root/.mixxx /dist-linux/bin/mixxx /music/BiteDJ_Test_Groove_128BPM.wav /music/BiteDJ_Test_Techno_124BPM.wav --resourcePath /dist-linux/share/mixxx/ --full-screen --style Fusion & \
        sleep 3 && \
        DISPLAY=:99 wmctrl -r Mixxx -b add,fullscreen 2>/dev/null || true && \
        tail -f /dev/null \
    "

echo "============================================================"
echo "  SUCCESS! BiteDJ is running in Docker test container."
echo ""
echo "  -> Web UI (noVNC):  http://localhost:6080/vnc.html"
echo "  -> Live Audio:     http://localhost:8000/stream.mp3"
echo "  -> Direct VNC:     vnc://localhost:5900"
echo "  -> Test Music:     Loaded from /music into library"
echo "============================================================"
