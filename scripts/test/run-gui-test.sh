#!/usr/bin/env bash
set -euo pipefail

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
DIST_DIR="${REPO_DIR}/dist-linux"
MUSIC_DIR="${BITEDJ_TEST_MUSIC_DIR:-${REPO_DIR}/test-music}"
source "${REPO_DIR}/scripts/test/gui-test-settings.sh"
for port in "${BITEDJ_TEST_WEB_PORT:-}" "${BITEDJ_TEST_AUDIO_PORT:-}" "${BITEDJ_TEST_VNC_PORT:-}"; do
    if [[ -n "${port}" ]] && [[ ! "${port}" =~ ^[1-9][0-9]{0,4}$ || "${port}" -lt 1 || "${port}" -gt 65535 ]]; then
        echo "Test ports must be integers between 1 and 65535." >&2
        exit 1
    fi
done
EXTRA_DOCKER_ARGS=(--label "us.bitedj.test.kind=gui")
if [[ -n "${BITEDJ_TEST_USB_DIR:-}" ]]; then
    if [[ ! -d "${BITEDJ_TEST_USB_DIR}" ]]; then
        echo "BITEDJ_TEST_USB_DIR must name an existing directory." >&2
        exit 1
    fi
    EXTRA_DOCKER_ARGS+=(-v "$(cd "${BITEDJ_TEST_USB_DIR}" && pwd):/media/TestUSB:ro")
fi
# Never replace another checkout's live test container.
if docker container inspect "${CONTAINER_NAME}" >/dev/null 2>&1; then
    verify_test_instance_owner
fi

if [ ! -f "${DIST_DIR}/bin/mixxx" ]; then
    echo "Error: dist-linux/bin/mixxx not found!"
    echo "Please build the ARM64 binary first with: ./scripts/build/docker-build.sh --platform linux/arm64"
    exit 1
fi

# Automated runs get disposable settings, deterministic music, and no host ports.
# Reuse the launcher ownership convention without replacing a manual instance.
if [[ "${BITEDJ_TEST_AUTOMATED:-0}" == 1 ]]; then
    if docker container inspect "${CONTAINER_NAME}" >/dev/null 2>&1; then
        echo "Automated tests require a fresh instance name." >&2
        exit 1
    fi
    docker image inspect bitedj-gui-test:latest >/dev/null
    docker run -d --name "${CONTAINER_NAME}" \
        --label "us.bitedj.test.worktree=${REPO_DIR}" \
        --label "us.bitedj.test.branch=$(git -C "${REPO_DIR}" branch --show-current)" \
        --label "us.bitedj.test.kind=e2e" \
        -v "${DIST_DIR}:/dist-linux:ro" \
        -v "${REPO_DIR}/res/skins/BiteDJ:/dist-linux/share/mixxx/skins/BiteDJ:ro" \
        -v "${REPO_DIR}/res/effects/chains:/dist-linux/share/mixxx/effects/chains:ro" \
        -v "${MUSIC_DIR}:/music:ro" \
        -v "${REPO_DIR}/tests/e2e/fixtures:/fixtures:ro" \
        -v "${REPO_DIR}/tests/e2e/start.sh:/start-e2e.sh:ro" \
        -v "${REPO_DIR}/scripts/test/audio_stream.py:/audio_stream.py:ro" \
        bitedj-gui-test:latest bash /start-e2e.sh
    exit
fi

echo "==> 1. Synchronizing effect chains and resources to dist-linux..."
mkdir -p "${DIST_DIR}/share/mixxx/effects/chains"
cp -r "${REPO_DIR}/res/effects/chains/"*.xml "${DIST_DIR}/share/mixxx/effects/chains/"
cp -R "${REPO_DIR}/res/effects/rekordbox7" "${DIST_DIR}/share/mixxx/effects/"
cp -r "${REPO_DIR}/res/skins/BiteDJ" "${DIST_DIR}/share/mixxx/skins/"

echo "==> 2. Ensuring test music tracks exist..."
if [ ! -f "${MUSIC_DIR}/BiteDJ_Test_Groove_128BPM.wav" ]; then
    (cd "${REPO_DIR}" && python3 scripts/test/generate_test_music.py)
fi

echo "==> 3. Building/verifying BiteDJ GUI test container..."
if [[ "${BITEDJ_TEST_REBUILD_IMAGE:-0}" == 1 ]] || ! docker image inspect bitedj-gui-test:latest >/dev/null 2>&1; then
    docker build --build-arg "BITEDJ_BUILDER_IMAGE=${BITEDJ_BUILDER_IMAGE:-bitedj-builder-linux-arm64:latest}" -t bitedj-gui-test:latest -f "${REPO_DIR}/docker/gui-test.Dockerfile" "${REPO_DIR}"
fi

echo "==> 4. Launching BiteDJ GUI test instance (1024x600, VNC + PulseAudio)..."
if docker container inspect "${CONTAINER_NAME}" >/dev/null 2>&1; then
    docker exec "${CONTAINER_NAME}" pkill -9 mixxx 2>/dev/null || true
    docker rm -f "${CONTAINER_NAME}"
fi

mkdir -p "${CONFIG_DIR}"
if [[ ! -f "${CONFIG_DIR}/mixxx.cfg" ]]; then
    printf '[Browse]\nQuickLinks /music/\n' > "${CONFIG_DIR}/mixxx.cfg"
fi

docker run -d \
    --name "${CONTAINER_NAME}" \
    --label "us.bitedj.test.worktree=${REPO_DIR}" \
    --label "us.bitedj.test.branch=$(git -C "${REPO_DIR}" branch --show-current)" \
    -p "127.0.0.1:${BITEDJ_TEST_WEB_PORT:-}:6080" \
    -p "127.0.0.1:${BITEDJ_TEST_AUDIO_PORT:-}:8000" \
    -p "127.0.0.1:${BITEDJ_TEST_VNC_PORT:-}:5900" \
    -v "${DIST_DIR}:/dist-linux:ro" \
    -v "${MUSIC_DIR}:/music:ro" \
    -v "${CONFIG_DIR}:/root/.mixxx:rw" \
    -v "${REPO_DIR}/scripts/test/audio_stream.py:/audio_stream.py:ro" \
    "${EXTRA_DOCKER_ARGS[@]}" \
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

printf '%s\n' "${CONTAINER_NAME}" > "${REPO_DIR}/test-config/active-instance"
WEB_PORT="$(test_host_port 6080)"
AUDIO_PORT="$(test_host_port 8000)"
VNC_PORT="$(test_host_port 5900)"
# Configure this container's noVNC audio widget, including cached GUI images.
docker exec "${CONTAINER_NAME}" python3 -c '
from pathlib import Path
import html, json, re, sys
p = Path("/usr/share/novnc/vnc.html")
s = p.read_text()
s = re.sub(r":8000(?=/stream[.]mp3)", ":" + sys.argv[1], s)
s = re.sub(r"<title>.*?</title>", "<title>BiteDJ — " + html.escape(sys.argv[2]) + "</title>", s, flags=re.S)
p.write_text(s)
Path("/usr/share/novnc/branch.json").write_text(json.dumps({
    "branch": sys.argv[2], "revision": sys.argv[3], "binarySha256": sys.argv[4]
}) + "\n")
' "${AUDIO_PORT}" "$(git -C "${REPO_DIR}" branch --show-current)" \
    "$(git -C "${REPO_DIR}" describe --always --dirty)" \
    "$(shasum -a 256 "${DIST_DIR}/bin/mixxx" | awk '{print $1}')"
echo "Instance: ${CONTAINER_NAME}"
echo "Branch: $(git -C "${REPO_DIR}" branch --show-current)"
echo "Web UI: http://localhost:${WEB_PORT}/vnc.html"
echo "Audio: http://localhost:${AUDIO_PORT}/stream.mp3"
echo "VNC: localhost:${VNC_PORT}"
echo "Settings: ${CONFIG_DIR}"
