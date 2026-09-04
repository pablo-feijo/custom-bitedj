#!/bin/bash
docker run --rm -v $(pwd):/out ubuntu:24.04 bash -c "
apt-get update && apt-get install -y libjxl0.7 librav1e0 libsvtav1enc1d1 libssh-gcrypt-4
cp /usr/lib/aarch64-linux-gnu/libjxl.so* /tmp/libs/ || true
cp /usr/lib/aarch64-linux-gnu/libjxl_threads.so* /tmp/libs/ || true
cp /usr/lib/aarch64-linux-gnu/librav1e.so* /tmp/libs/ || true
cp /usr/lib/aarch64-linux-gnu/libSvtAv1Enc.so* /tmp/libs/ || true
cp /usr/lib/aarch64-linux-gnu/libssh-gcrypt.so* /tmp/libs/ || true
tar -czvf /out/bitedj_libs2.tar.gz -C /tmp/libs .
"
