# Local GUI & Audio Testing Environment

For automated suite selection, assertions and coverage limits, see
[the test strategy](TESTING.md). The legacy FX/preview shell entry points now
run the isolated desktop smoke suite.

<!-- Modified for Custom Bite DJ on 2026-09-09: clarify fork identity and attribution. -->

This guide describes [Custom Bite DJ](../README.md), an independent fork of
[Team Deckshark’s BiteDJ](https://github.com/TeamDeckshark/bitedj), based on Mixxx.

Custom Bite DJ provides a local Docker environment for inspecting the 1024×600
skin and exercising virtual audio paths on macOS/Linux. It does not establish
Raspberry Pi hardware performance or upstream certification.

Use this workflow to **quickly test and verify changes before deploying to hardware or generating OS images**.

---

## Independent Branch Instances

Create a feature worktree from the agreed, existing semver integration branch:

```bash
git worktree add ../bitedj-my-feature -b codex/my-feature codex/v0.0.7
cd ../bitedj-my-feature
./scripts/build/docker-build.sh --platform linux/arm64
./scripts/test/run-gui-test.sh
```

Use the actual agreed base if the next semver branch has not been created yet.
The launcher chooses a worktree-specific container and free localhost ports.
It prints the noVNC, audio and VNC endpoints. Another worktree can run the same
commands concurrently; its writable build, install, settings and results stay separate.

For stable endpoints, choose an unused port set:

```bash
BITEDJ_TEST_INSTANCE=bitedj-xsploit-gui \
BITEDJ_TEST_WEB_PORT=6081 \
BITEDJ_TEST_AUDIO_PORT=8001 \
BITEDJ_TEST_VNC_PORT=5901 ./scripts/test/run-gui-test.sh
```

The launcher remembers the instance in `test-config/active-instance`. Settings
live in `test-config/<instance>/`, results in `test-results/<instance>/`.
The FX and preview scripts select that instance and discover its mapped ports.
Container ownership labels prevent either launching or testing against another
worktree/branch by mistake. The legacy instance on port 6080 is left alone.

To inspect or restart only your instance:

```bash
source ./scripts/test/gui-test-settings.sh
verify_test_instance_owner
docker port "$CONTAINER_NAME"
docker exec "$CONTAINER_NAME" pkill -9 mixxx
# Relaunch with the same explicit port variables to retain fixed endpoints:
BITEDJ_TEST_WEB_PORT=6081 BITEDJ_TEST_AUDIO_PORT=8001 \
BITEDJ_TEST_VNC_PORT=5901 ./scripts/test/run-gui-test.sh
```

Restarting replaces only the owned container and preserves its settings directory.
Default automatic ports may change on restart. Use a separate worktree for each
branch; do not switch a checkout underneath a running test instance. No host USB
volume is mounted by default. Set `BITEDJ_TEST_USB_DIR=/path/to/export` to mount
a chosen export read-only at `/media/TestUSB`. Rebuild the shared GUI image when required with
`BITEDJ_TEST_REBUILD_IMAGE=1`; already-running containers retain their images.

The examples below describe the historical fixed-port setup. Substitute the
owned `$CONTAINER_NAME` and printed endpoints. This checkout provides
`scripts/test/test-gui-fx.sh` and `scripts/test/test-gui-preview.sh`; references below to
`test-gui-automated.sh` describe the older workflow, whose script is absent here.
Their coordinate-based checks need review against the current skin; screenshot
capture and HTTP checks alone do not prove correct DSP. Use the native
`EffectSlotTest` for control-to-audio verification.

---

## 1. Architecture Overview

```
                      +-------------------------------------------------------+
                      |               Docker Test Container                   |
                      |            (bitedj-gui-test-instance)                 |
                      |                                                       |
                      |  +--------------------+     +---------------------+  |
                      |  |   Mixxx (BiteDJ)   |     |     PulseAudio      |  |
                      |  |   1024x600 Fusion  |---->|   (virtual sink)    |  |
                      |  +--------------------+     +----------+----------+  |
                      |            |                           |              |
                      |            v                           v              |
                      |  +--------------------+     +---------------------+  |
                      |  |    Xvfb / Openbox  |     |   audio_stream.py   |  |
                      |  |   (:99 Display)    |     |    (FFmpeg MP3)     |  |
                      |  +--------------------+     +----------+----------+  |
                      |            |                           |              |
                      |            v                           |              |
                      |  +--------------------+                |              |
                      |  |  x11vnc :5900      |                |              |
                      |  |  websockify :6080  |                |              |
                      |  +--------------------+                |              |
                      +------------|---------------------------|--------------+
                                   |                           |
                 http://localhost:6080/              http://localhost:8000/
                                   v                           v
                        +--------------------+      +--------------------+
                        |  Browser (noVNC)   |      | Live Audio Player  |
                        |  1024x600 Touch GUI|      | 192k MP3 Stream    |
                        +--------------------+      +--------------------+
```

- **Screen Resolution**: Locked to 1024x600, matching this fork’s target touchscreen resolution.
- **Audio Subsystem**: PulseAudio virtual dummy sink routing PortAudio audio directly to an internal HTTP MP3 streaming server.
- **Pre-Loaded Test Music**: Synthesized 44.1kHz stereo test tracks (`BiteDJ_Test_Groove_128BPM.wav` and `BiteDJ_Test_Techno_124BPM.wav`) are mounted into `/music` and automatically cued onto Deck 1 and Deck 2.
- **Browser Accessibility**: Web-based noVNC interface with auto-scaling, direct VNC, and in-browser audio player.

---

## 2. Interactive Testing (`scripts/test/run-gui-test.sh`)

To spin up the container and interact with the UI manually:

```bash
./scripts/test/run-gui-test.sh
```

### Endpoints
| Service | URL | Description |
| :--- | :--- | :--- |
| **Web UI (noVNC)** | `http://localhost:6080/` | Full BiteDJ touch GUI (auto-connects and auto-scales) |
| **noVNC Direct** | `http://localhost:6080/vnc.html` | Standard noVNC client |
| **noVNC Lite** | `http://localhost:6080/vnc_lite.html` | Minimalist client without app toolbar |
| **Live Audio Stream** | `http://localhost:8000/` | Web player streaming live Mixxx master audio |
| **Raw MP3 Stream** | `http://localhost:8000/stream.mp3` | Direct audio endpoint for VLC / mpv / curl |
| **Native VNC** | `vnc://localhost:5900` | For desktop VNC clients (TigerVNC, RealVNC, Screen Sharing) |

### Key Controls in Test GUI
- **Play / Pause**: Press `d` for Deck 1, press `l` for Deck 2.
- **Rewind / Cue**: Click at the very start of the bottom overview waveform (left for Deck 1, right for Deck 2), or press `f` (Deck 1 cue), `;` (Deck 2 cue).
- **Effect Unit**:
  1. Click the top dropdown in the right panel to pick an effect (e.g., Distortion, Reverb, Echo).
  2. Click `1` or `2` to assign the effect to Deck 1 or Deck 2.
  3. Click the `OFF` button to toggle it to `ACTIVE` (red).
  4. Turn the large `SUPER` knob to adjust effect depth/spread (drives all linked parameters).
  5. Turn the large `MIX` knob to blend wet/dry audio.

---

## 3. Automated Pre-Deployment Suite (`test-gui-automated.sh`)

To run an end-to-end automated verification without manual clicking:

```bash
./test-gui-automated.sh
```

### What It Tests:
1. **Container Health**: Verifies container is running or boots it up.
2. **Network Endpoints**: Asserts HTTP 200 on noVNC (`:6080/vnc.html`), ES modules (`:6080/core/rfb.js`), and Live Audio (`:8000/`).
3. **Audio Stream Throughput**: Reads live MP3 stream data and verifies audio buffers are flowing from PulseAudio.
4. **Mixxx Window Geometry**: Verifies Xvfb window is running at exactly 1024x600 resolution.
5. **Track Cueing**: Seeks both decks to position 0:00 and captures proof screenshot.
6. **FX Rack DSP Activation**: Selects an effect from the dropdown, assigns to Deck 1, turns state to `ACTIVE`, and sets `MIX` to 100%.
7. **Playback Verification**: Triggers playback on Deck 1, validates elapsed time progression, and verifies active audio bytes over the MP3 stream.

### Proof Artifacts
Each run outputs visual verification screenshots to `test-results/`:
- `test-results/01_cued.png`: Decks cued at start.
- `test-results/02_fx_active.png`: FX unit configured and active.
- `test-results/03_playing.png`: Active playback waveform scrolling.

---

## 4. Recommended Release & Pre-Deployment Workflow

Before pushing code to a live Raspberry Pi or building an OS image, always follow this pipeline:

```bash
# 1. Compile the ARM64 binary into dist-linux/
./scripts/build/docker-build.sh --platform linux/arm64

# 2. Run the automated GUI & audio test suite
./test-gui-automated.sh

# 3. (Optional) Open browser for manual listen/touch check
open http://localhost:6080/
open http://localhost:8000/

# 4. Deploy:
# Option A: Hot-deploy binary to live Pi over SSH
./scripts/deploy/deploy-ssh.sh

# Option B: Generate complete, flashable OS image
./scripts/build/generate-pi-image.sh
```

---

## 5. Hardware Controller Passthrough on macOS

When running BiteDJ in the local Docker environment on macOS, USB controllers (such as the Pioneer DDJ-400) connect via macOS CoreMIDI. Because Docker Desktop runs inside a hypervisor VM without direct host USB audio/MIDI device passthrough, a lightweight native bridge connects CoreMIDI to Mixxx's PortMIDI backend:

1. **Mac CoreMIDI Bridge**: A native CoreMIDI listener forwards hardware MIDI packets to the container over TCP port 5004 and routes Mixxx's LED/VU meter messages back to the physical controller.
2. **Mixxx Device Discovery**: Open the BiteDJ GUI in noVNC (`http://localhost:6080/`), navigate to **Settings -> Devices**, and tap **Rescan**. The Pioneer DDJ-400 will appear in the device list ready for live mixing.

---

## 6. Headless Debugging & Precision UI Coordinate Guide

When writing tests or automating UI interactions in `bitedj-gui-test-instance`:

### A. Tooling Quick Reference
| Task | Command | Forbidden / Non-functional |
| :--- | :--- | :--- |
| **Kill Mixxx** | `pkill -9 mixxx` | `killall mixxx` (command not found) |
| **Take Screenshot** | `DISPLAY=:99 scrot /tmp/screen.png` | `import` (command not found) |
| **Crop Screenshot** | `ffmpeg -y -i in.png -vf "crop=W:H:X:Y" out.png` | Python `PIL` / `cv2` (libraries not installed) |
| **Skin Updates** | Edit `res/skins/` and copy to `dist-linux/share/mixxx/skins/` | Modifying `/dist-linux` inside container (read-only mount) |

### B. Exact 1024x600 Coordinate Grid
- **Main Tabs (`topbar.xml`)**: `y=20`
  - `PLAY` (Overview): `x=100`
  - `BROWSE` (Library): `x=300`
  - `SAMPLER`: `x=500`
  - `LEVELS`: `x=700`
  - `SETTINGS`: `x=950`
- **Settings Sub-Tabs**: `y=60` (bar `y=40..80`)
  - `GENERAL`: `x=73`, `LIBRARY`: `x=219`, **`PAD FX`: `x=366`**, `DEVICE`: `x=512`, `AUDIO`: `x=658`, `SYSTEM`: `x=805`, `INFO`: `x=951`
  - PAD FX is the third visible option. Named triggers preserve the existing saved WidgetStack indices. Its editor uses the bottom area with 16px outer horizontal and 12px bottom padding; the deck footer is hidden only on this tab.
- **General Settings (`settings.xml`)**: mixer/playback on the left; display and cleanup on the right.

| Row | Center y | Left | Right |
| --- | --- | --- | --- |
| 0 | 104 | Crossfader | Wave |
| 1 | 156 | Deck 1 assignment | Apply Waveform EQ |
| 2 | 208 | Deck 2 assignment | Palette |
| 3 | 260 | EQ Mode | Key |
| 4 | 312 | Jog | Grid |
| 5 | 364 | Vinyl Brake | Clear |
| 6 | 416 | Hot Cue | Played |
| 7 | 470 | Track Load | Return to Play |

Two-button centers: left `374, 458`; right `886, 970`. Three-button centers:
left `360, 416, 472`; right `872, 928, 984`. Track Load: `290, 350, 410, 470`.
Phrases toggle: `788,104` (default On); click twice to verify Off then On, including paused decks and restart persistence. Return to Play: Off `886,470`, On `970,470` (default Off).
Played reset: `932`. See [the canonical option/control map](../AGENTS.md#d-settings---general-options-x73-y60)
for each button's value and key. Standard rows are 52px; Track Load is 58px.
The General footer starts at `y=520`; PAD FX alone hides it.

Update this guide, the root mapping and affected automation in the same commit
whenever UI options change. Remeasure after layout changes; these positions
are for the current 1024×600 skin.

Library column settings use independent visibility and width controls with
48px row spacing. See the [verified Library mapping](../AGENTS.md#e-settings---library-options-x219-y60)
and [Library screenshot](UI_SCREENSHOTS.md#settings-library); the Preview row
is at `y=390`, with ON/OFF at `820` and L width at `988`.

### C. Zero-Dependency Pixel Scanning Recipe
Dump a screenshot to PPM and scan raw RGB values in standard Python to find exact widget bounds before issuing `xdotool` clicks:
```bash
ffmpeg -y -i /tmp/screen.png /tmp/screen.ppm 2>/dev/null
python3 -c "
with open('/tmp/screen.ppm', 'rb') as f:
    f.readline(); dims = f.readline()
    while dims.startswith(b'#'): dims = f.readline()
    w, h = map(int, dims.split()); f.readline(); data = f.read()
# Scan pixel buffer data[(y * w + x) * 3]
"
```

General Settings typography: labels 12px, segment/action text 11px; button
geometry and the coordinate mappings above are unchanged. This leaves clearance
for “3 Band” on the 1024×600 display.

### Rekordbox and Prepare fixtures

Use the [synthetic fixture generator and procedure](../tests/rekordbox/README.md). Browse root rows are Prepare `y=52`, Computer `75`, History `99`, Rekordbox `121`; the expanded fixture child is `143`. Add tracks with **Add to Prepare** in their context menu. In Prepare, use **Move Up**, **Move Down**, or **Remove**; verify order after restarting. Loading retains queue entries and never starts playback automatically.

Overview previews show A–H for hot cues and 1–8 for memories; the Play waveform keeps full names. Keep memory numbers above the optional 10px phrase strip. Check both views with Phrases Off/On and Day/Night. Store generated media, screenshots, recordings, statistics and reports only in ignored test-results paths.

Compact overview cue labels retain the cue color as a small badge with contrasting text. Verify both hot-cue letters and memory numbers in Day/Night, with phrases enabled and disabled. Fixtures include distinct cue colors to make regressions visible.

For top-tab automation, move to the measured button center, wait briefly for the fullscreen menu to settle, then click. Capture again if the menu bar shifts the layout; do not reuse coordinates from a shifted screenshot.

Bottom-preview cue priority: use 2px colored marker lines with a contrasting border, painted above the countdown watermark. Keep cue letters/numbers and phrase labels at 8px. Verify hot cues and memories remain distinct with phrases On/Off and in Day/Night.

The main `cue_point` is shown as an orange **CUE** marker in both bottom previews, matching Play (`#ff6000`). It remains visible when the playhead is exactly on the cue. Verify this separately from hot-cue letters and memory-cue numbers, with phrases On/Off.

At overlapping positions, the orange main **CUE** line and label paint last, above hot cues and memory cues. Keep the CUE label unabridged; cue metadata and existing edit targets are unchanged. Test exact overlaps with a hot cue and a memory cue separately.

## Documentation screenshots

The [0.0.7 UI gallery](UI_SCREENSHOTS.md) contains publication images, linked
from the README and changelog. These curated assets are the exception to the
rule excluding raw test screenshots from Git. Agent refresh requirements live
in [the canonical screenshot policy](../AGENTS.md#published-ui-screenshots).

1. Build and launch an isolated worktree with the commands above. Use the
   inherited application version; do not bump it for a documentation change.
2. Use the launcher's synthetic music or the [Rekordbox fixture](../tests/rekordbox/README.md).
   Load both decks and let analysis finish. Pause playback for stable captures.
3. Open Browse, choose the synthetic music folder/export and make the waveform
   **Preview** column visible in Settings → Library (ON/OFF: `820,390`;
   L width: `988,390`, measured in the current 1024×600 capture). Return to
   Browse and confirm the table contains actual waveforms and both deck previews.
4. Close dialogs and menus; wait for notifications and analysis overlays to clear.
   Keep the native 1024×600 window, with the Night theme for the primary gallery.
5. Run `./scripts/test/capture-ui-docs.sh`. It verifies instance ownership, captures
   Play, the prepared Browse table and all seven Settings tabs with `scrot`, and
   saves raw PNGs plus source/binary provenance in ignored
   `test-results/<instance>/ui-docs/`. It leaves the instance on Play.
6. View every PNG at full size. Confirm the selected tab, text, waveform previews,
   padding and footer: PAD FX alone hides the Settings deck footer. Reject blank,
   loading, tooltip-covered or unintended pages. For styling changes also inspect
   Day mode and publish additional images when needed to explain the change.
7. Copy only reviewed publication PNGs into `docs/images/ui/0.0.7/`. Update the
   gallery's source revision, captions and README previews; link the affected
   gallery anchors from the UI change's changelog entry. Verify PNG dimensions and
   all relative links. Never copy logs, settings, databases or audio into docs.

During 0.0.7 development, refresh its gallery in place. After release, preserve
it and its image directory; create a separate gallery and image directory for
the next version and point new changelog/README links there.
