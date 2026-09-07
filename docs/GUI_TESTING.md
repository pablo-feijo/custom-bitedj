# Local GUI & Audio Testing Environment

BiteDJ provides a local Docker-based testing environment that reproduces the Raspberry Pi's exact display resolution, skin layout, audio pipeline, and effect processing on your development machine (macOS/Linux).

Use this workflow to **quickly test and verify changes before deploying to hardware or generating OS images**.

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

- **Screen Resolution**: Locked to 1024x600, matching the official BiteDJ multi-touch screen.
- **Audio Subsystem**: PulseAudio virtual dummy sink routing PortAudio audio directly to an internal HTTP MP3 streaming server.
- **Pre-Loaded Test Music**: Synthesized 44.1kHz stereo test tracks (`BiteDJ_Test_Groove_128BPM.wav` and `BiteDJ_Test_Techno_124BPM.wav`) are mounted into `/music` and automatically cued onto Deck 1 and Deck 2.
- **Browser Accessibility**: Web-based noVNC interface with auto-scaling, direct VNC, and in-browser audio player.

---

## 2. Interactive Testing (`run-gui-test.sh`)

To spin up the container and interact with the UI manually:

```bash
./run-gui-test.sh
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
./docker-build.sh --platform linux/arm64

# 2. Run the automated GUI & audio test suite
./test-gui-automated.sh

# 3. (Optional) Open browser for manual listen/touch check
open http://localhost:6080/
open http://localhost:8000/

# 4. Deploy:
# Option A: Hot-deploy binary to live Pi over SSH
./deploy-ssh.sh

# Option B: Generate complete, flashable OS image
./generate-pi-image.sh
```
