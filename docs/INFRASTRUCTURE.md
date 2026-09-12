# Infrastructure & Build Architecture

<!-- Modified for Custom Bite DJ on 2026-09-09: clarify fork identity and attribution. -->

This guide describes [Custom Bite DJ](../README.md), an independent fork of
[Team Deckshark’s BiteDJ](https://github.com/TeamDeckshark/bitedj), based on Mixxx.

Unlike upstream Mixxx which is distributed as a standard desktop application, BiteDJ is engineered as a full appliance operating system (OS) optimized for the Raspberry Pi. This requires a significantly different build and deployment infrastructure.

## 1. Cross-Compilation Engine
Upstream Mixxx relies on standard native `CMake` builds for Windows, macOS, and Linux. 
BiteDJ utilizes a Dockerized cross-compilation pipeline to build ARM64 binaries from any host machine (macOS/x86_64).
- **`docker/build.Dockerfile`**: Provides a reproducible Debian Trixie (13) environment pre-loaded with an ARM64 cross-compiler (`aarch64-linux-gnu-g++`).
- **`scripts/build/docker-build.sh`**: Automates the invocation of the Docker container, mounting the local source tree, and compiling the binary into the `dist-linux/` folder.
- **Legacy library extraction**: `scripts/legacy/get_libs.sh` and `scripts/legacy/get_libs2.sh` retain the older Ubuntu ARM64 runtime-library extraction workflow. They write archives under ignored `test-results/legacy-libs/` and are not used by the current build.

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
- **Environment**: Cleaned up legacy Qt scaling flags, retaining only the absolute essentials: `WLR_DRM_NO_MODIFIERS=1` and `QT_WAYLAND_SHELL_INTEGRATION=xdg-shell`.

## 7. Kernel & Boot Optimizations
BiteDJ modifies `/boot/firmware/cmdline.txt` during the `mixxx-pi-gen` OS generation to configure the Raspberry Pi hardware explicitly for real-time audio and kiosk presentation:
- **CPU Governor**: Forces `cpufreq.default_governor=performance` so the CPU never throttles down, ensuring consistent, low-latency audio processing.
- **Kernel Preemption**: Appends `preempt=full` to the kernel boot string, allowing audio callback threads to forcefully interrupt lower-priority system processes.
- **Silent Boot (Plymouth)**: Appends `quiet splash logo.nologo vt.global_cursor_default=0` to hide Linux boot text and the blinking cursor. The fixed firmware splash is disabled because it cannot distinguish HDMI from portrait-native DSI. Kernel boot keeps DSI native, the custom theme includes a pre-rotated candidate, and Sway alone applies the later 90-degree application transform. The physical Pi 5 splash remains portrait-cropped; evidence and next diagnostics are in the pinned image recipe's [boot splash investigation](../mixxx-pi-gen/docs/BOOT_SPLASH.md).
- **USB Audio Default**: Injects a custom `soundconfig.xml` to force `DDJ-400: USB Audio` as the default Master and Headphone output upon first boot.
- **USB Maximum Current**: Modifies `/boot/firmware/config.txt` to include `max_usb_current=1`. This boosts the maximum USB current allowance from 600mA to 1200mA (1.2A), which is mandatory to simultaneously power the Pioneer DDJ-400 controller and the HDMI touchscreen display without voltage drops or disconnects.


## 8. Drivers & Hardware Dependencies
BiteDJ runs directly on the hardware with a minimal set of underlying drivers:
- **Audio Subsystem**: Mixxx routes audio dynamically depending on the selected interface. The DDJ-400 CUE uses bit-perfect PortAudio directly over ALSA (`snd-usb-audio`), while Bluetooth output strictly leverages the virtual `pipewire` ALSA node. We explicitly disabled strict hardware polling (`PA_ALSA_PLUGHW`) because accessing the internal headphone hardware (`bcm2835`) directly causes a fatal `VCHI` kernel panic on the Raspberry Pi.
- **Graphics Subsystem**: Hardware acceleration is provided by the `vc4-kms-v3d` DRM driver (configured in `config.txt`). Mixxx's fast-scrolling waveforms utilize OpenGL rendered natively on the GPU.
- **Compositor**: `sway` (wlroots) is the Wayland compositor. Touchscreen input translates natively into Wayland gestures.
- **Frameworks**: 
  - `qt6-wayland` and `qt6-qpa-plugins` are required for native Wayland UI execution.
  - `xwayland` provides the X11 compatibility bridge (`xcb`) for legacy dialogs and VST integrations when native Wayland touch drag-and-drop requires mitigation.
- **Storage**: Automounting of DJ USB drives is handled by `udiskie`, relying on `polkitd` rules for passwordless operation. Do not also start `devmon`: the two automounters can race and mount one partition at two paths.
- **Networking & Wireless**: WiFi and Bluetooth connectivity is managed entirely by native OS dialogs launched seamlessly over the UI. This requires:
  - `network-manager-gnome` (provides `nm-connection-editor` for WiFi selection).
  - `blueman` (provides `blueman-manager` for Bluetooth audio pairing).
  - `wvkbd` (Wayland virtual keyboard triggered explicitly via C++ when launching these connection dialogs).

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
env 
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
workspace 1

# 5. Auto-execute BiteDJ on compositor startup
exec "WLR_DRM_NO_MODIFIERS=1 QT_WAYLAND_SHELL_INTEGRATION=xdg-shell /usr/bin/bitedj --resourcePath /usr/share/mixxx/ --full-screen --style Fusion"
```

## Touch date/time, boot clocks and restart

Settings → Info keeps four dashboard cards plus an SSH remote-access control.
The SSH panel queries `systemctl is-enabled ssh.service` without elevation and
uses `sudo -n systemctl enable --now ssh.service` or
`sudo -n systemctl disable --now ssh.service` for the selected transition.
The image remains key-only: enabling the daemon does not enable password login.
Disabling is immediate and ends existing remote maintenance access. Missing
units, sudo failures and service errors remain visible in the fullscreen panel.

Tap Local Time for a timezone-first editor: Region and City / timezone select
an installed IANA zone and preview its
local date and time. Apply calls `timedatectl set-timezone` before updating sync;
a failed zone change stops the sequence. A timezone-only edit preserves the
clock instant and never issues `set-time`. The dashboard updates without restart.

Automatic sync uses `timedatectl set-ntp`. Turning it off reveals a calendar date
picker and 24-hour step controls. Explicit manual edits disable sync before
`timedatectl set-time`; nonexistent local times at daylight-saving transitions
are rejected. Failed calls remain visible, including disabling sync without
successfully setting time. No shell parses user values. Commands are asynchronous
and bounded to 15 seconds. Preview selection and Cancel never alter OS settings. On Linux, non-root clock
mutations use `sudo -n timedatectl` with structured arguments; read-only queries
remain unprivileged. This uses the Pi image's existing passwordless sudo policy
without installing broader permissions or waiting for a Polkit/password dialog.
Systems without that authorization report the command failure.

Settings → System → Overclock reads `/boot/firmware/config.txt`, falling back to
`/boot/config.txt`, on Raspberry Pi 4 Model B and Pi 5 Model B. The editor shows
saved boot overrides, which can differ from currently running clocks. CPU range
is 1000–2400 MHz on Pi 4 and 1000–3000 MHz on Pi 5; GPU range is 400–1000 MHz;
`over_voltage` is 0–6 (25 mV steps). These are editor bounds, not stability
certifications. Zero removes the corresponding override and lets firmware choose.
Firmware defaults saves removal of all three overrides immediately; custom
values use Save for next restart. Cooling and board-dependent stability remain
relevant; no new preset is presented as hardware-validated.
See [Raspberry Pi's clock documentation](https://www.raspberrypi.com/documentation/computers/config_txt.html#overclocking-options).

Saving preserves unrelated boot options, places the managed overrides in `[all]`,
and retains the first original as `config.txt.bitedj-backup`. File operations run
on a worker; `QSaveFile` commits the replacement atomically. A stale editor refuses
to overwrite an externally changed file. Included configurations, advanced voltage
or forced-turbo overrides, unsupported conditional clock sections, unknown boards,
and out-of-range existing values are refused. Recovery after boot failure is to
restore the backup on the boot partition using another computer. Saving never
restarts automatically; Restart system is a separate confirmed action.

For the real boot paths, non-root saving invokes `sudo -n` with the same BiteDJ
executable in `--bitedj-apply-boot-settings` mode. This headless helper exits
before GUI/audio/library initialization, reads the fixed OS boot path again,
checks the expected SHA-256, validates the three numeric settings, and performs
the backup and atomic save as root. It accepts neither a destination filename
nor arbitrary file contents. Normal user settings remain owned by the session
user. Missing sudo authorization is reported rather than prompting for a password.

System → Power / Restart offers Restart BiteDJ, Restart system and Power off.
Application restart exits the event loop, destroys the main window and core
services (flushing settings and recordings), then launches the same executable
with its arguments and inherited environment. System restart/power-off use
`sudo -n systemctl reboot` / `sudo -n systemctl poweroff` for the non-root
desktop user, with failed starts, nonzero exits and timeouts reported. This
fixes Restart system from both Power and Overclock without relying on a graphical
Polkit agent. Desktop containers without systemd expose an unavailable/error state.


### Audio scheduling during browsing

GUI, Qt pool and rendering-driver threads start with normal Linux scheduling.
Only audio (FIFO 70), the worker scheduler (62), track readers (60) and controller
input (50) opt into real-time priorities. Starting the GUI at FIFO 49 also caused
Mesa and some Qt workers to inherit FIFO, allowing browse/render work to consume
the shared real-time budget. Analysis/history retain background scheduling.
Verify with `ps -L -p PID -o tid,cls,rtprio,comm` while scrolling using the controller:
Main/rendering must be TS; mixxx-engine must remain FF 70 when permissions allow.
Pair this with recorded audio and underrun counters; priority alone is not proof
of uninterrupted playback. Bluetooth transport dropouts require separate checks.
