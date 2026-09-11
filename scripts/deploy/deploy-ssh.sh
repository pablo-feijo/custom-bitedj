#!/usr/bin/env bash
set -euo pipefail

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "${REPO_DIR}"

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <raspberry_pi_ip>"
    echo "Example: $0 192.168.18.157"
    exit 1
fi

PI_IP=$1
PASSWORD="bitedj"
BINARY_PATH="dist-linux/bin/mixxx"
SHARE_PATH="dist-linux/share/mixxx"

echo "============================================================"
echo "  BiteDJ Hot-Deployer"
echo "============================================================"

echo "1. Cross-compiling BiteDJ (ARM64)..."
./scripts/build/docker-build.sh --platform linux/arm64

if [ ! -f "$BINARY_PATH" ]; then
    echo "ERROR: Compilation failed. $BINARY_PATH not found."
    exit 1
fi

echo "2. Transferring binary and ALL resources (skins, controllers, etc.) to Pi ($PI_IP)..."
expect -c "
set timeout -1
spawn scp -o StrictHostKeyChecking=no $BINARY_PATH pi@${PI_IP}:/tmp/bitedj
expect \"*?assword:*\"
send \"${PASSWORD}\r\"
expect eof
spawn scp -o StrictHostKeyChecking=no -r $SHARE_PATH pi@${PI_IP}:/tmp/mixxx_share
expect \"*?assword:*\"
send \"${PASSWORD}\r\"
expect eof
"

echo "3. Installing binary and resources, then restarting graphical session..."
expect -c "
set timeout 30
spawn ssh -o StrictHostKeyChecking=no pi@${PI_IP} \"echo ${PASSWORD} | sudo -S mv /tmp/bitedj /usr/bin/bitedj && echo ${PASSWORD} | sudo -S chmod +x /usr/bin/bitedj && echo ${PASSWORD} | sudo -S rm -rf /usr/share/mixxx && echo ${PASSWORD} | sudo -S mv /tmp/mixxx_share /usr/share/mixxx && echo ${PASSWORD} | sudo -S sh -c 'grep -q WLR_NO_HARDWARE_CURSORS /etc/environment || echo WLR_NO_HARDWARE_CURSORS=1 >> /etc/environment; if test -c /dev/dri/renderD128; then grep -q ^WLR_RENDER_DRM_DEVICE= /etc/environment || echo WLR_RENDER_DRM_DEVICE=/dev/dri/renderD128 >> /etc/environment; fi' && echo ${PASSWORD} | sudo -S systemctl restart lightdm\"
expect \"*?assword:*\"
send \"${PASSWORD}\r\"
expect eof
"

echo "============================================================"
echo "  SUCCESS! BiteDJ has been deployed to $PI_IP and is restarting."
echo "============================================================"
