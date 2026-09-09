#!/usr/bin/env bash
set -euo pipefail
export DISPLAY=:99 QT_AUTO_SCREEN_SCALE_FACTOR=0 QT_ENABLE_HIGHDPI_SCALING=0 QT_SCALE_FACTOR=1.0
export BITEDJ_SETTINGS_PATH=/root/.mixxx
mkdir -p "$BITEDJ_SETTINGS_PATH"
cp /fixtures/mixxx.cfg /fixtures/soundconfig.xml "$BITEDJ_SETTINGS_PATH/"
pulseaudio -D --exit-idle-time=-1 --system=false
pactl load-module module-null-sink sink_name=bitedj_test
pactl set-default-sink bitedj_test
pactl set-default-source bitedj_test.monitor
python3 /audio_stream.py > /tmp/audio.log 2>&1 &
Xvfb :99 -screen 0 1024x600x24 > /tmp/xvfb.log 2>&1 &
for attempt in {1..100}; do
    if xdpyinfo >/dev/null 2>&1; then break; fi
    sleep 0.1
done
openbox > /tmp/openbox.log 2>&1 &
x11vnc -display :99 -forever -shared -nopw > /tmp/vnc.log 2>&1 &
websockify --web /usr/share/novnc 6080 localhost:5900 > /tmp/web.log 2>&1 &
exec /dist-linux/bin/mixxx /music/tone.wav --resourcePath /dist-linux/share/mixxx/ --full-screen --style Fusion
