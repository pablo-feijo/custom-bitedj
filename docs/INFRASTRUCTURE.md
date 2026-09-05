# Infrastructure & Build Architecture

Unlike upstream Mixxx which is distributed as a standard desktop application, BiteDJ is engineered as a full appliance operating system (OS) optimized for the Raspberry Pi. This requires a significantly different build and deployment infrastructure.

## 1. Cross-Compilation Engine
Upstream Mixxx relies on standard native `CMake` builds for Windows, macOS, and Linux. 
BiteDJ utilizes a Dockerized cross-compilation pipeline to build ARM64 binaries from any host machine (macOS/x86_64).
- **`Dockerfile`**: Provides a reproducible Debian Trixie (13) environment pre-loaded with an ARM64 cross-compiler (`aarch64-linux-gnu-g++`).
- **`docker-build.sh`**: Automates the invocation of the Docker container, mounting the local source tree, and compiling the binary into the `dist-linux/` folder.
- **Dependency Fetching**: The scripts `get_libs.sh` and `get_libs2.sh` manually download and extract Debian ARM64 package headers directly from `deb.debian.org` (e.g., `libasound2-dev`, `libqt6waylandclient6`). This eliminates the need for a massive, complex sysroot.

## 2. OS Image Generation (`mixxx-pi-gen`)
BiteDJ ships as a complete, flashable `.img` file. We use a heavily customized git submodule fork of `pi-gen` (the official tool used to build Raspberry Pi OS).
- **Custom Stages**: We injected `stage3/02-desktop` to strip out the standard PIXEL desktop and replace it with **Sway (Wayland)**. We explicitly disabled the `apt autoremove` steps in this stage to prevent UI packages (like `lightdm` and `waybar`) from being accidentally purged.
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
- **USB Maximum Current**: Modifies `/boot/firmware/config.txt` to include `max_usb_current=1`. This boosts the maximum USB current allowance from 600mA to 1200mA (1.2A), which is mandatory to simultaneously power the Pioneer DDJ-400 controller and the HDMI touchscreen display without voltage drops or disconnects.


## 8. Drivers & Hardware Dependencies
BiteDJ runs directly on the hardware with a minimal set of underlying drivers:
- **Audio Subsystem**: While `pipewire-audio` is installed for the desktop, BiteDJ explicitly bypasses the sound server using PortAudio over ALSA (`env PA_ALSA_PLUGHW=1`) to communicate directly with the `snd-usb-audio` driver. This guarantees bit-perfect, ultra-low latency routing to the DDJ-400 hardware.
- **Graphics Subsystem**: Hardware acceleration is provided by the `vc4-kms-v3d` DRM driver (configured in `config.txt`). Mixxx's fast-scrolling waveforms utilize OpenGL rendered natively on the GPU.
- **Compositor**: `sway` (wlroots) is the Wayland compositor. Touchscreen input translates natively into Wayland gestures.
- **Frameworks**: 
  - `qt6-wayland` and `qt6-qpa-plugins` are required for native Wayland UI execution.
  - `xwayland` provides the X11 compatibility bridge (`xcb`) for legacy dialogs and VST integrations when native Wayland touch drag-and-drop requires mitigation.
- **Storage**: Automounting of DJ USB drives is handled by `udevil` and `udiskie`, relying on `polkitd` rules for passwordless operation.

## 9. Modern Library Dependencies (Trixie OS Requirements)
The shift from Debian Bookworm to Debian Trixie (13) was strictly mandated by the modern dependencies of the cross-compiled `v0.0.3` binary. Attempting to run the compiled binary on older distributions will result in `cannot open shared object file` or `version not found` errors. 

Key dependencies that require the Trixie environment include:
- **Core C/C++**: `libc6` (GLIBC 2.38+) and `libstdc++6` (GLIBCXX 3.4.32+).
- **Qt6 Ecosystem**: `libqt6core6` (Qt 6.8+), `libqt6gui6`, `libqt6widgets6`, `libqt6network6`, `libqt6opengl6`, `libqt6svg6`, `libqt6xml6`, `libqt6sql6`, and crucially `libqt6core5compat6` (Qt6 Core 5 Compat module).
- **Audio Decoding & Processing**: 
  - `libavcodec61`, `libavformat61`, `libavutil59`, `libswresample5` (Requires FFmpeg 7+, incompatible with Bookworm's FFmpeg 5).
  - `libflac14` (Requires `libFLAC.so.14`, incompatible with Bookworm's `libflac12`).
  - `libebur128-1` (For EBU R128 loudness analysis).
  - `libtag2` (TagLib 2.x for metadata parsing).
- **DSP & Synthesis**: `libfftw3-double3`, `libfftw3-single3`, `librubberband2t64`, `libsoundtouch1v5`.
- **Hardware & MIDI**: `libportmidi0`, `libupower-glib3`, `libhidapi-hidraw0`.
- **Other Media/Formats**: `libprotobuf-lite32t64`, `libshout-idjc3`, `libopusfile0`, `libmad0`, `libmodplug1`, `libwavpack1`.

When building local test environments (e.g., Docker NoVNC containers) or verifying the OS image, all of these libraries must be explicitly satisfied.

## 10. Declarative System Configurations
To ensure reproducible builds and easy debugging, the following explicit configurations and environment variables are strictly enforced across the OS to ensure the display, autologin, and USB subsystems function correctly:

### LightDM Autologin (Headless Kiosk)
**File**: `/etc/lightdm/lightdm.conf`
Forces the Pi to bypass the login screen and instantly drop the `pi` user into the Sway Wayland compositor.
```ini
[Seat:*]
autologin-user=pi
autologin-session=sway
```

### USB Power (Controller + Display)
**File**: `/boot/firmware/config.txt`
Crucial for powering the DDJ-400 and an HDMI touchscreen simultaneously without undervolting.
```ini
max_usb_current=1
```

### Kernel Real-Time & Visuals
**File**: `/boot/firmware/cmdline.txt`
Prioritizes audio processing over system tasks and completely silences the boot text for a clean aesthetic.
```text
preempt=full cpufreq.default_governor=performance quiet splash logo.nologo vt.global_cursor_default=0
```

### Sway & Qt Wayland Environment
When launching BiteDJ (either via autostart or debugging over SSH), the following environment block is required to bind the binary to the active Wayland session and disable legacy modifiers:
```bash
# Required to route display output to the running Sway session
export SWAYSOCK=$(ls /run/user/1000/sway-ipc.*.sock | head -n 1)
export WAYLAND_DISPLAY=wayland-1

# Launch BiteDJ with explicit hardware and styling flags
env PA_ALSA_PLUGHW=1 \
    WLR_DRM_NO_MODIFIERS=1 \
    QT_WAYLAND_SHELL_INTEGRATION=xdg-shell \
    /usr/bin/bitedj --resourcePath /usr/share/mixxx/ --full-screen --style Fusion
```

### Sway Window Manager (Kiosk Compositor)
**File**: `~/.config/sway/config` (or injected globally via `/etc/sway/config`)
Sway handles the actual display logic. The configuration is stripped of typical desktop features to enforce a strict, single-app kiosk experience:

```text
# 1. Disable the default Sway top bar to prevent users from escaping the app
bar {
    mode invisible
}

# 2. Set the custom splash/background image natively
output * bg /usr/share/backgrounds/bitedj-wallpaper.jpg fill

# 3. Disable all window decorations (titlebars, borders)
default_border none
default_floating_border none

# 4. Force default workspace to prevent Sway from splitting window spaces
workspace $ws1

# 5. Auto-execute BiteDJ on compositor startup
exec "env PA_ALSA_PLUGHW=1 WLR_DRM_NO_MODIFIERS=1 QT_WAYLAND_SHELL_INTEGRATION=xdg-shell /usr/bin/bitedj --resourcePath /usr/share/mixxx/ --full-screen --style Fusion"
```
