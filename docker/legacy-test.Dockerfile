FROM debian:trixie

ENV DEBIAN_FRONTEND=noninteractive
ENV LIBGL_ALWAYS_SOFTWARE=1

RUN apt-get update && apt-get install -y \
    xvfb x11vnc novnc websockify fluxbox \
    libqt6core6 libqt6gui6 libqt6widgets6 libqt6network6 \
    libqt6opengl6 libqt6svg6 libqt6xml6 libqt6sql6 libqt6core5compat6 \
    libflac14 libavcodec61 libavformat61 libavutil59 libswresample5 \
    libtag2 libsndfile1 libportaudio2 libusb-1.0-0 \
    && rm -rf /var/lib/apt/lists/*

RUN echo '#!/bin/bash\n\
export DISPLAY=:0\n\
Xvfb :0 -screen 0 1280x800x24 &\n\
sleep 2\n\
fluxbox &\n\
x11vnc -display :0 -nopw -listen localhost -xkb -forever -shared &\n\
websockify --web /usr/share/novnc/ 8080 localhost:5900 &\n\
sleep 2\n\
/usr/bin/bitedj --resourcePath /usr/share/mixxx/ --full-screen --style Fusion\n\
wait' > /start.sh && chmod +x /start.sh

EXPOSE 8080
CMD ["/start.sh"]
