#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DIST_DIR="${SCRIPT_DIR}/dist-linux"

if [ ! -f "${DIST_DIR}/bin/mixxx" ]; then
    echo "Error: dist-linux/bin/mixxx not found!"
    exit 1
fi

echo "==> Building BiteDJ GUI test container..."
docker build -t bitedj-gui-test -f - "${SCRIPT_DIR}" << 'DOCKERFILE_EOF'
FROM bitedj-builder:latest

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    xvfb \
    openbox \
    x11vnc \
    novnc \
    websockify \
    wmctrl \
    xdotool \
    && rm -rf /var/lib/apt/lists/*

EXPOSE 6080
DOCKERFILE_EOF

echo "==> Launching BiteDJ test container at 1024x600..."
docker rm -f bitedj-gui-test-instance 2>/dev/null || true
docker run -d \
    --name bitedj-gui-test-instance \
    -p 6080:6080 \
    -v "${DIST_DIR}:/dist-linux:ro" \
    bitedj-gui-test:latest \
    bash -c "\
        Xvfb :99 -screen 0 1024x600x24 & \
        sleep 2 && \
        DISPLAY=:99 openbox & \
        x11vnc -display :99 -forever -shared -nopw -bg & \
        websockify --web /usr/share/novnc 6080 localhost:5900 & \
        sleep 2 && \
        DISPLAY=:99 QT_AUTO_SCREEN_SCALE_FACTOR=0 QT_ENABLE_HIGHDPI_SCALING=0 QT_SCALE_FACTOR=1.0 BITEDJ_SETTINGS_PATH=/tmp/bitedj /dist-linux/bin/mixxx --safe-mode --full-screen & \
        sleep 3 && \
        DISPLAY=:99 wmctrl -r Mixxx -b add,fullscreen || true && \
        tail -f /dev/null \
    "

echo "============================================================"
echo "  BiteDJ is running in Docker!"
echo "  Open your browser to: http://localhost:6080/vnc.html"
echo "============================================================"
