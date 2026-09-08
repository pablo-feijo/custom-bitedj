#!/bin/bash
docker run --rm -v $(pwd):/out ubuntu:24.04 bash -c "
apt-get update && apt-get install -y libflac12 libtag1v5 libavcodec60 libavformat60 libavutil58 libswresample4
mkdir -p /tmp/libs
cp /usr/lib/aarch64-linux-gnu/libFLAC.so.12* /tmp/libs/
cp /usr/lib/aarch64-linux-gnu/libtag.so.1* /tmp/libs/
cp /usr/lib/aarch64-linux-gnu/libavcodec.so.60* /tmp/libs/
cp /usr/lib/aarch64-linux-gnu/libavformat.so.60* /tmp/libs/
cp /usr/lib/aarch64-linux-gnu/libavutil.so.58* /tmp/libs/
cp /usr/lib/aarch64-linux-gnu/libswresample.so.4* /tmp/libs/
cp /usr/lib/aarch64-linux-gnu/libaom.so.* /tmp/libs/ || true
cp /usr/lib/aarch64-linux-gnu/libdav1d.so.* /tmp/libs/ || true
cp /usr/lib/aarch64-linux-gnu/libx265.so.* /tmp/libs/ || true
cp /usr/lib/aarch64-linux-gnu/libx264.so.* /tmp/libs/ || true
cp /usr/lib/aarch64-linux-gnu/libvpx.so.* /tmp/libs/ || true
cp /usr/lib/aarch64-linux-gnu/libmp3lame.so.* /tmp/libs/ || true
cp /usr/lib/aarch64-linux-gnu/libvulkan.so.* /tmp/libs/ || true
tar -czvf /out/bitedj_libs.tar.gz -C /tmp/libs .
"
