# BiteDJ Custom OS - Networking & Audio Infrastructure

This document serves as a complete, versioned record of the custom UI and low-level audio fixes applied to the Raspberry Pi OS build to make it touch-friendly and resolve Bluetooth/ALSA conflicts.

## 1. Custom Touch-Friendly Networking GUIs

The default `blueman-manager` and `nmtui` managers were notoriously buggy and difficult to use on small touchscreens. Both have been completely replaced with custom hybrid `zenity` wrappers that directly interface with the low-level system daemons.

### Wi-Fi Manager (`bitedj-wifi.sh`)
- **Location:** `mixxx-pi-gen/stage3/02-desktop/files/bitedj-wifi.sh`
- **Behavior:**
  - Automatically suspends Mixxx fullscreen to draw over the UI.
  - Triggers a raw `nmcli device wifi list` scan.
  - Renders a touch-friendly `zenity --list` matrix of available SSIDs and their signal strengths.
  - On selection, prompts for a password via an obscured `zenity --password` modal.
  - Connects securely using the NetworkManager CLI.

### Bluetooth Manager (`bitedj-bt.sh`)
- **Location:** `mixxx-pi-gen/stage3/02-desktop/files/bitedj-bt.sh`
- **Behavior:** 
  - Completely bypasses `blueman-manager` for connecting known devices.
  - Queries `bluetoothctl devices Paired` (updated for BlueZ 5.66+).
  - Displays a massive 1-tap list of paired headphones.
  - Automatically injects an explicit `disconnect` command before `connect` to guarantee the A2DP profile initializes without throwing "Device or Resource Busy" DBus errors.
  - Includes a pinned `[Disconnect Current Device]` option to instantly route audio back to the physical outputs.
  - Includes a `[Forget a Device...]` fallback utilizing `bluetoothctl remove`.

---

## 2. Audio Engine Stability Fixes (PortAudio + PipeWire)

Routing Mixxx's Master output to a Bluetooth headset while simultaneously routing the CUE output to the raw DDJ-400 hardware presented massive timing and resource conflicts on Linux. The following fixes were permanently embedded into the OS builder.

### WirePlumber 0.5 SPA-JSON Fix
- **Location:** `mixxx-pi-gen/stage3/02-desktop/files/wireplumber/wireplumber.conf.d/51-ignore-ddj400.conf`
- **Problem:** WirePlumber aggressively hijacked the DDJ-400, causing a resource collision when Mixxx tried to claim it, resulting in the playhead completely freezing.
- **Solution:** Migrated the old Lua script into the new WirePlumber 0.5 `SPA-JSON` format. The daemon now completely ignores `alsa_card.*DDJ-400*`, granting Mixxx 100% exclusive access.

### PipeWire Clock-Sync Injection
- **Location:** `mixxx-pi-gen/stage3/02-desktop/files/i3.conf`
- **Problem:** When PortAudio tried to talk to the `default` PipeWire server, their memory clocks misaligned, generating thousands of `paOutputUnderflow` errors per second.
- **Solution:** Injected `env PIPEWIRE_LATENCY="1024/44100"` into the Sway `bitedj` launch string. This forcefully synchronizes the daemon's clock with PortAudio, eliminating buffer underruns over wireless connections.

### BCM2835 VCHI Kernel Panic Fix (Removed PA_ALSA_PLUGHW)
- **Problem:** In previous versions (v0.0.3), we forced PortAudio to use raw hardware polling (`PA_ALSA_PLUGHW=1`) to reduce latency. However, selecting the physical Raspberry Pi Headphone jack (`bcm2835`) under these strict hardware parameters triggered a fatal `VCHI service connection (status=-11)` kernel panic, crashing the Pi.
- **Solution:** We completely stripped `PA_ALSA_PLUGHW=1` from all launchers in v0.0.5. PortAudio now correctly interacts with the virtual ALSA stack, preventing the physical hardware driver from panicking.

### UI Device Filtering & Custom Naming
- **Location:** `src/preferences/audiodevicesettings.cpp`
- **Problem:** Removing the hardware flag caused the confusing virtual `"pipewire"` and `"default"` nodes to appear in the backend, but the Bite DJ UI filter strictly dropped them, hiding the Bluetooth audio option entirely.
- **Solution:** The C++ array insertion logic was rewritten to explicitly whitelist the `"pipewire"` and `"sysdefault"` virtual ALSA nodes. When detected, the C++ engine dynamically overrides their display names to **"PipeWire / Bluetooth"** in the custom touchscreen skin, granting the user a completely intuitive audio routing selection without touching the dangerous physical hardware nodes.

---

## 3. Sway Workspace Ghosting Fix
- **Location:** `mixxx-pi-gen/stage3/02-desktop/files/i3.conf`
- **Problem:** The Sway compositor was given a literal quoted string (`set $ws1 "1:BiteDJ"`). Sway took the quotes literally, spawning the actual workspace as `1:BiteDJ` while instantly forcing focus to a ghost workspace named `"1:BiteDJ"`, resulting in a completely blank screen on boot.
- **Solution:** Stripped all literal strings and replaced the variable with a raw integer (`workspace 1`). BiteDJ now reliably maps fullscreen onto the active screen at boot.
