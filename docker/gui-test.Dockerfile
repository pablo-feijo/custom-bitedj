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
    scrot \
    x11-utils \
    scrot \
    pulseaudio \
    pulseaudio-utils \
    libasound2-plugins \
    ffmpeg \
    curl \
    procps \
    && sed -i '1s/^/export let supportsWebCodecsH264Decode = false;\\n/' /usr/share/novnc/core/util/browser.js \
    && sed -i "s/supportsWebCodecsH264Decode = await _checkWebCodecsH264DecodeSupport();/supportsWebCodecsH264Decode = false;/" /usr/share/novnc/core/util/browser.js \
    && sed -i "s/import { dragThreshold, supportsWebCodecsH264Decode } from '.\/util\/browser.js';/import { dragThreshold } from '.\/util\/browser.js?v=20260906'; const supportsWebCodecsH264Decode = false;/" /usr/share/novnc/core/rfb.js \
    && sed -i 's|\"./app/ui.js\"|\"./app/ui.js?v=20260906\"|g' /usr/share/novnc/vnc.html \
    && sed -i 's|\"../core/rfb.js\"|\"../core/rfb.js?v=20260906\"|g' /usr/share/novnc/app/ui.js \
    && sed -i "s|'./core/rfb.js'|'./core/rfb.js?v=20260906'|g" /usr/share/novnc/vnc_lite.html \
    && echo '<!DOCTYPE html><html><head><meta http-equiv=\"refresh\" content=\"0; url=/vnc.html?autoconnect=true&resize=scale&v=20260906_audio\"></head><body><a href=\"/vnc.html?autoconnect=true&resize=scale&v=20260906_audio\">BiteDJ Web UI</a></body></html>' > /usr/share/novnc/index.html \
    && rm -rf /var/lib/apt/lists/*

COPY scripts/test/novnc_audio_snippet.html /tmp/novnc_audio_snippet.html
RUN python3 -c "with open('/usr/share/novnc/vnc.html', 'r') as f: c = f.read(); s = open('/tmp/novnc_audio_snippet.html', 'r').read(); open('/usr/share/novnc/vnc.html', 'w').write(c.replace('</body>', s + '\n</body>'))" && rm /tmp/novnc_audio_snippet.html


EXPOSE 5900 6080 8000
