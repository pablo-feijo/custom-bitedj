# Infrastructure & Build Architecture

Unlike upstream Mixxx which is distributed as a standard desktop application, BiteDJ is engineered as a full appliance operating system (OS) optimized for the Raspberry Pi. This requires a significantly different build and deployment infrastructure.

## 1. Cross-Compilation Engine
Upstream Mixxx relies on standard native `CMake` builds for Windows, macOS, and Linux. 
BiteDJ utilizes a Dockerized cross-compilation pipeline to build ARM64 binaries from any host machine (macOS/x86_64).
- **`Dockerfile`**: Provides a reproducible Debian Bookworm environment pre-loaded with an ARM64 cross-compiler (`aarch64-linux-gnu-g++`).
- **`docker-build.sh`**: Automates the invocation of the Docker container, mounting the local source tree, and compiling the binary into the `dist-linux/` folder.
- **Dependency Fetching**: The scripts `get_libs.sh` and `get_libs2.sh` manually download and extract Debian ARM64 package headers directly from `deb.debian.org` (e.g., `libasound2-dev`, `libqt6waylandclient6`). This eliminates the need for a massive, complex sysroot.

## 2. OS Image Generation (`mixxx-pi-gen`)
BiteDJ ships as a complete, flashable `.img` file. We use a heavily customized git submodule fork of `pi-gen` (the official tool used to build Raspberry Pi OS).
- **Custom Stages**: We injected `stage3/02-desktop` to strip out the standard PIXEL desktop and replace it with **Sway (Wayland)**.
- **Auto-Kiosk Mode**: The OS is configured to auto-login the `pi` user and immediately launch `bitedj` via Sway without any display manager (GDM/LightDM).
- **Filesystem & Swap**: We dynamically strip out non-essential Pi packages (like `rpi-swap` and `rpi-usb-gadget`) to maximize performance and SD card lifespan.

## 3. Wayland vs. X11
Upstream Mixxx Linux builds generally target X11. BiteDJ targets Wayland for tear-free touchscreen performance.
- **Key Libraries**: BiteDJ requires `qt6-wayland` and `libwayland-client0`.
- **XWayland Bridge**: While the UI renders natively via Qt Wayland, some legacy dialogs and VST integrations still rely on XWayland. The taskbar (`Waybar`) wrapper scripts explicitly pass `QT_WAYLAND_SHELL_INTEGRATION=xdg-shell` to ensure correct rendering.

## 4. CMake Build Flags
BiteDJ compiles with a specific subset of features to keep the binary lightweight for the Pi:
- Video mixing, external vinyl control (Serato/Traktor), and broadcast (Shoutcast) modules are strictly managed or disabled to save CPU cycles.
- Compiles with `-O3` and specific ARM64 tuning flags for maximum real-time audio performance.

## 5. OS-Level Access Control (Polkit & Udisks2)
Because BiteDJ operates as a kiosk without a keyboard, standard Linux permission prompts for hardware operations must be bypassed securely.
- **USB Automount / Eject**: BiteDJ injects a custom `polkit` rule (`50-udisks.rules`) into the OS. This explicitly allows the `pi` user passwordless access to `org.freedesktop.udisks2.*` actions. This enables DJs to safely tap "Eject" on their USB drives within the BiteDJ UI without a root password dialog appearing on the headless screen.

## 6. Autostart & Environment Variables
The Sway compositor handles BiteDJ's execution automatically on boot. 
- **Flags**: `bitedj` is launched directly with `--full-screen` and `--style Fusion` via `i3.conf`. We explicitly removed legacy flags like `--safe-mode` that interfered with loading user preferences.
- **Environment**: Cleaned up legacy Qt scaling flags, retaining only the absolute essentials: `PA_ALSA_PLUGHW=1` (for PortAudio raw hardware access), `WLR_DRM_NO_MODIFIERS=1`, and `QT_WAYLAND_SHELL_INTEGRATION=xdg-shell`.

## 7. Kernel & Boot Optimizations
BiteDJ modifies `/boot/firmware/cmdline.txt` during the `mixxx-pi-gen` OS generation to configure the Raspberry Pi hardware explicitly for real-time audio and kiosk presentation:
- **CPU Governor**: Forces `cpufreq.default_governor=performance` so the CPU never throttles down, ensuring consistent, low-latency audio processing.
- **Kernel Preemption**: Appends `preempt=full` to the kernel boot string, allowing audio callback threads to forcefully interrupt lower-priority system processes.
- **Silent Boot (Plymouth)**: Appends `quiet splash logo.nologo vt.global_cursor_default=0` to completely hide the Linux boot text and blinking cursor. A custom Plymouth script (`bitedj.script`) is injected to display a high-resolution Pioneer splash image seamlessly during boot.
- **USB Audio Default**: Injects a custom `soundconfig.xml` to force `DDJ-400: USB Audio` as the default Master and Headphone output upon first boot.
