# Building and Deploying BiteDJ

This guide outlines the workflows for building the BiteDJ binary and generating the Raspberry Pi OS image.

## Prerequisites
- macOS or Linux development machine
- Docker Desktop installed and running
- SSH access to your Raspberry Pi (for hot deployments)

## 1. Generating a Complete OS Image
If you need to flash a brand-new Raspberry Pi, you must bake a complete OS image containing Sway, Wayland, and all of BiteDJ's dependencies. Note: The image must be based on Debian 13 (Trixie) to provide the modern libraries required by the binary (e.g. GLIBC 2.38, Qt 6.8, libFLAC 14, FFmpeg 7).

Run the image generator script:
```bash
./generate-pi-image.sh
```
This script will:
1. Automatically compile the Linux ARM64 binary via `docker-build.sh`.
2. Launch the `mixxx-pi-gen` Docker container.
3. Build a customized Debian Trixie OS from scratch (Stages 0-3).
4. Export a fully flashable `.zip` file into `mixxx-pi-gen/deploy/`.

**Flashing the SD Card:**
Once the `.zip` is generated, insert your SD card and run:
```bash
./flash-sdcard.sh
```
Follow the interactive prompts to safely write the image to your disk.

**Expanding the Filesystem (Important):**
Because the raw image is deliberately kept small (~6GB) to speed up flashing, it will not automatically fill large SD cards. If you encounter "No space left on device" errors over SSH, you must manually expand the root partition:
```bash
# Connect to the Pi and run:
sudo raspi-config nonint do_expand_rootfs
sudo resize2fs /dev/mmcblk0p2
```

## 2. Hot-Deploying the Binary via SSH
If you already have a working BiteDJ Raspberry Pi and just need to update the application code, you do NOT need to burn a new OS image. You can hot-swap the binary over SSH.

**Compile the binary locally:**
```bash
./docker-build.sh --platform linux/arm64
```
*This creates the executable inside `dist-linux/bin/`.*

**Deploy to the Pi:**
Use the included `deploy-ssh.sh` script or run:
```bash
scp dist-linux/bin/mixxx pi@<YOUR_PI_IP>:/tmp/bitedj
ssh pi@<YOUR_PI_IP> "sudo mv /tmp/bitedj /usr/bin/bitedj && sudo chmod +x /usr/bin/bitedj"

# Restart LightDM to cleanly reload the graphical session and prevent black screens:
ssh pi@<YOUR_PI_IP> "sudo systemctl restart lightdm"
```
LightDM will cleanly restart the Sway compositor and immediately launch the new BiteDJ binary without crashing into a failed state.
