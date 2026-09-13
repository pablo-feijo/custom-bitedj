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

The canonical roadmap for additional resolutions and visible decks 3/4 is
[Responsive display and four-deck plan](DISPLAY_AND_DECK_LAYOUT_PLAN.md).

Use this workflow to **quickly test and verify changes before deploying to hardware or generating OS images**.

---

## Independent Branch Instances

Create a feature worktree from the agreed, existing semver integration branch.
Use the [initial-base exception](BRANCH_VERSIONING.md#starting-work) while the
new integration branch is still unpublished:

```bash
git fetch origin
git worktree add ../bitedj-my-feature -b codex/my-feature origin/codex/v0.0.8
cd ../bitedj-my-feature
# Assign the branch prerelease version per BRANCH_VERSIONING.md before building.
./scripts/build/docker-build.sh --platform linux/arm64
./scripts/test/run-gui-test.sh
```

Use the actual agreed base if the next semver branch has not been created yet.
The launcher chooses a worktree-specific container and free localhost ports.
It prints the noVNC, audio and VNC endpoints. Another worktree can run the same
commands concurrently; its writable build, install, settings and results stay separate.

### Environment variables for launching and opening a branch preview

Run from the task worktree. Use a fresh shell when moving between tasks, or
unset previously exported `BITEDJ_TEST_*` overrides first: an exported instance
name takes precedence over `test-config/active-instance`. The launcher reads
process environment variables; it does **not** automatically load `.env` files.
Do not put task-specific exports in a global shell profile.

```bash
# Optional: choose a unique name; otherwise the worktree-derived name is used.
export BITEDJ_TEST_INSTANCE=bitedj-my-feature
# Optional: export unused fixed ports. Omit these for automatic allocation.
# export BITEDJ_TEST_WEB_PORT=6081
# export BITEDJ_TEST_AUDIO_PORT=8001
# export BITEDJ_TEST_VNC_PORT=5901
./scripts/test/run-gui-test.sh

# Discover the actual endpoints after every launch, including automatic ports.
source ./scripts/test/gui-test-settings.sh
verify_test_instance_owner || exit 1
export BITEDJ_TEST_WEB_URL="http://localhost:$(test_host_port 6080)/vnc.html"
export BITEDJ_TEST_AUDIO_URL="http://localhost:$(test_host_port 8000)/stream.mp3"
export BITEDJ_TEST_VNC_URL="vnc://localhost:$(test_host_port 5900)"
printf '%s\n' "$BITEDJ_TEST_WEB_URL" "$BITEDJ_TEST_AUDIO_URL" "$BITEDJ_TEST_VNC_URL"
# macOS; Linux desktop users can use xdg-open instead:
open "$BITEDJ_TEST_WEB_URL"
```

The URL variables above are shell conveniences, not launcher inputs. Agents
opening a Codex browser panel should pass the discovered web URL to the browser
open tool. Complete the [noVNC delivery gate](#novnc-delivery-gate) before handing
a preview URL to the user. Never reuse a URL from another task or an earlier
container launch.

| Launcher variable | Behavior |
| --- | --- |
| `BITEDJ_TEST_INSTANCE` | Explicit owned container name; otherwise remembered or derived per worktree. |
| `BITEDJ_TEST_WEB_PORT`, `BITEDJ_TEST_AUDIO_PORT`, `BITEDJ_TEST_VNC_PORT` | Optional host ports; unset means Docker chooses free localhost ports. |
| `BITEDJ_TEST_MUSIC_DIR` | Optional existing fixture directory; default is worktree `test-music/`. Supply both startup WAV tracks when overriding. |
| `BITEDJ_TEST_USB_DIR` | Optional existing directory mounted read-only at `/media/TestUSB`. |
| `BITEDJ_TEST_REBUILD_IMAGE=1` | Rebuild the shared GUI image when needed. |
| `BITEDJ_BUILDER_IMAGE` | Builder image used when building the GUI image. |

Keep `dist-linux/`, build outputs and settings inside the task worktree; do not
redirect them to another branch's mutable artifacts. Automated mode is reserved
for the test harness and does not publish browser ports.

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
`scripts/test/test-gui-fx.sh` and `scripts/test/test-gui-preview.sh`.
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

- **Screen Resolution**: Defaults to 1024x600 for the original HDMI touchscreen
  and coordinate-based regression suite. Set `BITEDJ_TEST_GEOMETRY=1280x720`
  with a separate `BITEDJ_TEST_INSTANCE` for visual checks of Raspberry Pi
  Touch Display 2 after its portrait-native DSI output is rotated to landscape.
  The launcher automatically matches the appliance's 1.20 Qt scale at that
  geometry; `BITEDJ_TEST_SCALE_FACTOR` can override it for a focused comparison.
  At 1.20, 44px logical targets render at about 53 physical pixels, and native
  menus/dialogs scale with the skin. Do not reuse 1024x600 coordinates at the
  larger resolution.
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

## 3. GUI and audio capture smoke

Start with two paused synthetic tracks, default Deck 1 FX routing and Night mode:

```bash
./scripts/test/capture-fx-smoke.sh
```

The script verifies owned-container health, discovered HTTP endpoints, nonempty
MP3 stream data and 1024×600 window geometry. It then requests cueing, selects
Standard ECHO through the two-column picker, and records clean/FX playback.
Inspect the resulting `01_cued.png`, `02_picker.png`, `03_fx_off.png` and
`04_fx_active.png`, plus `baseline.mp3` and `fx_audio.mp3`, under the instance's
ignored `test-results/<instance>/` directory.

These captures need review: HTTP success does not validate JavaScript syntax,
and different MP3 bytes do not prove DSP behavior. The focused native
`EffectSlotTest` checks effect activation, catalogue loading, beat timing,
bounded audio and stereo Ping Pong. Use the documentation capture helper for
reviewed gallery images, not these raw smoke artifacts.

---

## 4. Recommended Release & Pre-Deployment Workflow

Before pushing code to a live Raspberry Pi or building an OS image, always follow this pipeline:

```bash
# 1. Compile the ARM64 binary into dist-linux/
./scripts/build/docker-build.sh --platform linux/arm64

# 2. Run and review the GUI/audio capture smoke
./scripts/test/capture-fx-smoke.sh

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
Played reset: `932`. See [the canonical option/control map](../.agents/skills/bitedj-ui/references/settings.md#d-settings---general-options-x73-y60)
for each button's value and key. Standard rows are 52px; Track Load is 58px.
The General footer starts at `y=520`; PAD FX alone hides it.

Update this guide, the root mapping and affected automation in the same commit
whenever UI options change. Remeasure after layout changes; these positions
are for the current 1024×600 skin.

Library column settings use independent visibility and width controls with
48px row spacing. See the [verified Library mapping](../.agents/skills/bitedj-ui/references/settings.md#e-settings---library-options-x219-y60)
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

Use the [synthetic fixture generator and procedure](../tests/rekordbox/README.md). With a nonempty queue, Browse uses 28px rows: collapsed roots are Prepare `y=55`, Computer `83`, History `111`, Rekordbox `139`; its expanded fixture child is `167`. Each expanded child shifts subsequent rows by 28px. Add tracks with **Add to Prepare** in their context menu. In Prepare, use **Move Up**, **Move Down**, or **Remove**; verify order after restarting. Loading retains queue entries and never starts playback automatically.

Overview previews show A–H for hot cues and 1–8 for memories; the Play waveform keeps full names. Keep memory numbers above the optional 10px phrase strip. Check both views with Phrases Off/On and Day/Night. Store generated media, screenshots, recordings, statistics and reports only in ignored test-results paths.

Compact overview cue labels retain the cue color as a small badge with contrasting text. Verify both hot-cue letters and memory numbers in Day/Night, with phrases enabled and disabled. Fixtures include distinct cue colors to make regressions visible.

For top-tab automation, move to the measured button center, wait briefly for the fullscreen menu to settle, then click. Capture again if the menu bar shifts the layout; do not reuse coordinates from a shifted screenshot.

Bottom-preview cue priority: use 2px colored marker lines with a contrasting border, painted above the countdown watermark. Keep cue letters/numbers and phrase labels at 8px. Verify hot cues and memories remain distinct with phrases On/Off and in Day/Night.

The main `cue_point` is shown as an orange **CUE** marker in both bottom previews, matching Play (`#ff6000`). It remains visible when the playhead is exactly on the cue. Verify this separately from hot-cue letters and memory-cue numbers, with phrases On/Off.

At overlapping positions, the orange main **CUE** line and label paint last, above hot cues and memory cues. Keep the CUE label unabridged; cue metadata and existing edit targets are unchanged. Test exact overlaps with a hot cue and a memory cue separately.

## Documentation screenshots

The [0.0.8 UI gallery](UI_SCREENSHOTS_0.0.8.md) contains current publication images, linked
from the README and changelog. These curated assets are the exception to the
rule excluding raw test screenshots from Git. Agent refresh requirements live
in [the canonical screenshot policy](../.agents/skills/bitedj-ui/SKILL.md#published-ui-screenshots).

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
7. Copy only reviewed publication PNGs into `docs/images/ui/0.0.8/`. Update the
   gallery's source revision, captions and README previews; link the affected
   gallery anchors from the UI change's changelog entry. Verify PNG dimensions and
   all relative links. Never copy logs, settings, databases or audio into docs.

During 0.0.8 development, refresh its gallery in place. Keep the released
[0.0.7 gallery](UI_SCREENSHOTS.md) unchanged. After release, preserve
it and its image directory; create a separate gallery and image directory for
the next version and point new changelog/README links there.

### Docker cleanup and recovery

See [Docker maintenance](DOCKER_MAINTENANCE.md) for full prune, disk checks and
recovery after engine failure. A full prune removes stopped GUI instances and
may remove their unused image. Host `test-config/` remains; relaunch from the
correct worktree and use its freshly printed endpoints.

### Beat FX picker

At 1024×600, Play's FX tab is `(850,70)`, selector `(922,116)`, Deck 1/2
routing `(858,152)` / `(922,152)`, and activation `(986,152)`. The picker
stays inside `x=828..1015, y=100..599`. Its two effect columns have centers
`x=876,968`; seven row centers are `y=194,242,290,338,386,434,482`. Effect targets are
88px wide and 44px tall with 10px labels, with 4px gaps and outside padding.
Standard/Saved: `(876,119)` / `(968,119)`; Erase/Close: `(876,153)` / `(968,153)`; Prev/Next: `(876,538)` / `(968,538)`. Header/footer targets
are 30px tall; the page counter is 9px at `(922,514)`. Leaving FX closes the picker, including a skin reload.

Standard order is row-major, entries 1–14, then 15–25, listed in
[Beat FX](BEAT_FX.md). Saved holds legacy/custom entries. Native persisted IDs
remain independent of labels. Page/section changes and Close/Escape must leave
the selected chain unchanged. Selection starts a standard chain Off; the
existing Effect1 `enabled` control activates all its occupied slots together.
The `parameterN_beat_period` alias uses periods in beats and converts rate-based
parameters without changing saved raw values.

At the Touch Display 2 profile (1280x720 at 1.20), the panel occupies physical
`x=1039..1279`. The FX/Key/Jump/Grid centers are approximately
`(1071/1129/1186/1244,84)`, and the selector is `(1158,139)`. The contained
picker columns center at `x=1103/1213`; effect-row centers are
`y=232,290,348,406,464,522,580`. Standard/Saved are at `y=143`, Erase/Close at
`y=184`, and Prev/Next at `y=646`. Re-measure after a scale/profile change.

Run `scripts/test/capture-beatfx-docs.sh` with two paused synthetic tracks in
Night mode. Inspect both Standard pages and both Saved pages, especially Color
Filter versus Rhythmic Filter, selected highlighting, last-row clearance and
hidden empty cells. Publish reviewed Play/picker images in the
[current gallery](UI_SCREENSHOTS.md#beat-fx-picker). Also test Day through
Settings → System → Day `(980,312)`, then restore Night `(922,312)`; wait for
the skin reload/notification before using the top tabs (allow 9 seconds in the
Docker instance). Verify Clear and Escape
separately and confirm selecting an entry closes the picker without activating FX.

Focused native checks, including dialog-open geometry and page-change behavior:

```sh
./scripts/build/docker-build.sh --platform linux/arm64 --test-filter 'EffectSlotTest\.'
```

## Controller pad drawer

The Overview cue drawer follows the DDJ-400 mode selected on either deck.
Hot Cue restores the existing Hot Cues/Memory controls; Pad FX, Beat Jump and
Beat Loop show **touchable performance pads**. Touch or use the physical
pads to perform the displayed actions; no controller is required for touch. X still closes the drawer;
selecting a controller mode opens that deck again. Mode button release does
not close it. Other modes close only their own deck's visible drawer.

Runtime controls (not saved): `[PadFX],dN_mode` = 0 Hot Cue, 1 Pad FX 1,
2 Beat Jump, 3 Beat Loop, 4 Memory, 5 Pad FX 2; `dN_shift` = 0/1; `dN_jump_bank` = 0/1/2 for
1/16, 1, 16 multipliers. The existing mapping shares jump sizes across decks.

The header has separate 48×44px Previous/Next buttons and a read-only mode
label. A bounded 92px bank area holds two 44px pad rows and a 4px gap.
Navigation coordinates and mode mappings are listed below.
See [controller drawer screenshots](UI_SCREENSHOTS.md#controller-pad-drawer).

Touch pad centers retain x=`132,385,638,891`, y=`525,574` at 1024×600.
`[PadFX],dN_touch_p0..7` are momentary inputs, row-major. Pad FX reads saved
assignments on press, Beat Jump seeks once, Beat Loop holds rolls 1/4–2 and
toggles loops 4–32. Release, drawer hide, mode/Shift change and touch cancellation
release held actions. Release Echo's configured toggle remains latched until
its next press or Clear FX. `[PadFX],dN_hardware_p0..7` carries MIDI FX presses
(0 release, 1 Normal, 2 Shift) into the same system runtime, with independent
input ownership. `dN_hardware_clear` releases controller-owned FX on disconnect.
`runtime_available=1` selects this bridge; the mapping retains its old runtime
when used with older binaries. Shift Jump bank changes are shared with MIDI.

Pad FX 1 displays slots 0–7; Pad FX 2 displays slots 8–15, the existing saved
Shift assignments. FX 2 stays selected after Shift is released. Both touch
arrows and controller mode selection show these same assignments. MIDI FX 2
pads use notes `0x50..0x57` on the normal/shift pad channels, with note-on and
note-off routed to the second bank. No saved assignments or enum IDs migrate.

The legend follows saved Normal/Shift assignments, timing overrides, strength
and toggle settings. Beat Loop shows four held rolls and four toggle loops;
holding Shift shows its mapped shifted Pad FX assignments. Beat Jump's Shift
bank shows size ÷16 / ×16 on pads 7/8. Modes are independent per deck.

### noVNC module startup

The image build and launcher both run `scripts/test/repair-novnc.py`. It removes
an old malformed literal `\n` injection, avoids duplicate capability exports and
keeps the WebCodecs probe disabled. This also repairs cached GUI images without
rebuilding the application or replacing another task's container.
Run `python3 tests/novnc/test_repair.py` for the regression. Verify the served
JavaScript parses and connect in the browser; a desktop screenshot alone does
not verify the noVNC client. Reload a browser tab after repairing its served files.


### Waveform previews and touch drawer

Current header coordinates and control mappings are in the
[touch drawer guide](../.agents/skills/bitedj-ui/references/pads.md#touch-drawer-navigation-and-padding-1024600).
The Previous and Next buttons are separate from the read-only mode label;
update scripts that previously tapped the middle of the header to advance.
At 1024×600 use Previous `(116,476)`, Next `(944,476)`, Close `(994,476)`.
Both directions wrap through Hot Cues, Memory, Beat Jump, Pad FX and Beat Loop;
release events do not advance. Verify Deck 1 and Deck 2 independently.

For waveform checks, use the [native and cold-cache regression procedure](TESTING.md#preview-and-waveform-regressions).
With two paused tracks, change Wave at `(872|928|984,104)` and Palette at
`(886|970,208)`. Inspect Browse thumbnails, Play and both bottom deck summaries.
No full-screen display-mode notification/rebuild should occur for Palette.
Load the 16-second replacement fixture into a deck that held a 60-second track:
all four waveform sections must occupy the correct quarter of its summary.
Browse's unavailable-preview marker is an em dash, not a loading action.

### noVNC delivery gate

Before handing out a GUI link, run `python3 scripts/test/run-tests.py fast` and
`python3 scripts/test/run-tests.py e2e` with Node.js on PATH. The fast regression
covers fresh and cached noVNC sources and repeated repair; the desktop suite
parses every served JavaScript module. HTTP 200 is not sufficient validation.
Open the owned instance in a real browser and verify a connected desktop canvas,
not just the noVNC page shell. After changing noVNC assets, invalidate the module
graph cache and verify a previously opened tab reconnects. Never prepend feature
exports to upstream modules that already declare them.

The interactive launcher waits for Xvfb readiness before starting Openbox,
x11vnc and the application. Shell background jobs must not bypass this gate;
a browser connection refusal can mean x11vnc exited before the display existed.
### Browse touch navigation

The desktop E2E `test_autoplay_queue_controls` covers empty-queue feedback,
selected-track and whole-playlist appends, audible start, on/off state and
conditional options visibility. With both decks loaded, an empty queue is seeded from the decks (playing deck
first). With fewer than two loaded decks and no queue, the in-skin notification explains how to
load decks or add queued tracks. Native tests cover both stopped and right-deck
playing starts, plus audibility when consuming the last queued track.

Queue checks: select one track and tap **+ Queue** `(683,56)`, then open Folders
and verify Auto DJ appears with that track. Open a playlist and tap **Queue
All** `(780,56)`; all displayed tracks must append in order without changing
the selection or starting a deck. Verify **Auto Play** `(892,56)` turns on,
advances through the queue, and turns off while leaving current playback intact.
Verify the extra Auto DJ options row appears only while Auto Play is on and
collapses when off, including after a skin reload. Remove queued tracks and
check the Auto DJ root disappears once empty and off.
Check the queue again after restart to confirm persistence. Triple taps retain
the existing double-click behavior; queuing uses explicit buttons.


Use the [canonical Browse coordinates](../.agents/skills/bitedj-ui/references/settings.md#b-browse--library-navigation).
With an empty Prepare queue and the synthetic `/music` Quick Link configured,
tap Computer `(150,55)`, Quick Links `(150,83)`, then Music `(200,111)`.
A nonempty Prepare queue adds its row at `y=55`, shifting later rows down 28px. Group labels must expand
without switching to a table; a folder's indentation cell expands its
subfolders without opening its tracks. Tap its label to open tracks, then
Folders `(986,56)` to return. Drag a long tree without opening any row.
Headers at `y=73..90` use 9px bold text with 8px horizontal padding and
vertical centering; verify sorting at header center `y=82`, compact rows at
`y=102,124`, and footer visibility.
In Settings → Library, Preview enable is `(816,391)` and L width `(988,391)`.
Both are needed to show wide preview waveforms in a fresh test configuration.
Check Browse and the FX picker in Day and Night modes.

For the nested-folder swipe check, create synthetic empty directories before
starting the owned instance so its lazy directory cache sees them:

```sh
mkdir -p "test-music/Touch navigation/Nested"
for i in $(seq 1 24); do mkdir -p "test-music/Touch navigation/Nested/Folder $i"; done
```

After Computer and Quick Links, use expansion cells `(51,111)`, `(71,139)`
and `(91,167)` with an empty Prepare queue for Music, Touch navigation and Nested. Drag from `(300,459)`
to `(300,239)`; the tree must scroll and remain open. Restore the scroll and
tap Music's label to verify that a folder with children can still open tracks.

Verify Prepare is absent for an empty queue. Add a synthetic track through
**Add to Prepare**, return to Folders and confirm Prepare appears. Restart to
verify restoration, then remove the last queued track and confirm Prepare
disappears again. Loading a queued track does not remove it automatically.

Picker actions use 10px text: Erase, Close, Prev and Next. Erase and Close
share the second header row. Erase clears the current FX without deleting its
saved preset; the reserved `---` entry stays hidden. Standard/Saved
remain labeled tabs; the 9px page counter reads `1 / 2`. Saved has 14/8 entries.

## Service Deck preferences: jog smoothing

In the owned test instance, open service preferences with Ctrl+P, select Decks,
and maximize its window before using these verified 1024×600 coordinates.
The service dialog uses the native Fusion style independently of skin Day/Night.
Decks navigation is `(75,166)`; Deck options order is Cue mode, Intro start,
Track time display, Time Format, Track load point, Loading a track when deck is
playing, Clone deck, Jog-wheel smoothing. The new spin box is `(600,302)`, with
step arrows at `(965,298)` / `(965,307)`. Apply `(974,577)`, Cancel `(887,577)`,
OK `(800,577)`, Restore Defaults `(232,577)`.

`[Controls] JogWheelFilterLength` defaults to 6 and is bounded to 1–64. Check
that Up at 64 and Down at 1 do not exceed these bounds. Apply 64; edit 1 and
Cancel; reopening must retain 64. Save 17, quit normally to flush settings,
restart the owned binary, and verify 17. Restore Defaults must display 6;
Cancel reverts it until applied. Return the isolated test setting to 6 afterward.
The audio callback picks up an applied change without reopening tracks.

See the [service screenshot](UI_SCREENSHOTS.md#service-decks) and
[controller workflow](DDJ400_MAPPING.md#active-loop-jog-resizing). Hardware checks:
loop-active jog halves/doubles after 32 ticks, Shift jog only translates the grid,
and scratch release immediately resumes decks that were playing before touch.
Automated MIDI tests cover both decks and transition cases; real wheel feel and
physical controller timing still need hardware verification.

## Right-panel Grid editor

Overview right-panel order is FX, Key, Jump, Grid. Grid appends index 3 to
`[FxPanel],current`; existing saved FX/Key/Jump indices stay unchanged.
Selecting `[FxPanel],grid` enables `gridEditMode` on both waveforms: beat lines
become fully opaque and waveform dragging positions the track. Leaving Grid
restores the configured beat-line opacity and normal seek-disabled interaction.
Any active waveform drag is released on a mode change or when its page hides.

Each deck has a separate 168px block: deck label, Earlier/Later, Set grid here,
then BPM −/+. All action buttons are 44px high with 4px spacing and 11px labels;
tab labels use 10px. The combined FX layout uses a 204px side panel; recheck Grid clearance.

| Label | `[ChannelN]` native control | Action |
| --- | --- | --- |
| Earlier | `beats_translate_earlier` | Shift grid earlier |
| Later | `beats_translate_later` | Shift grid later |
| Set grid here | `beats_translate_curpos` | Align nearest beat to current playhead |
| BPM − | `beats_adjust_slower` | Reduce grid BPM by 0.01 |
| BPM + | `beats_adjust_faster` | Increase grid BPM by 0.01 |

Buttons emit momentary press/release, with no background repeat timer. Actions
require a loaded track with an editable beat grid. BPM actions edit the track's
grid, not the playback-rate slider. No controller mapping changes are required.

Grid deck headers display `[ChannelN],file_bpm` to two decimal places normally.
Training mode masks them as `?.?` and reveals the number only while the header
readout is held; BPM −/+ still adjust the editable grid normally.

## Training mode

In Settings → General, exercise the single Training Off, Line and Boxes selector.
At 1024×600 and 1280×720 confirm that the stacked scrolling waveforms are
replaced while the compact bottom-deck waves remain visible. Line spans four
major bar divisions, and Boxes draws exactly four boxes
per deck. The left source/key/loop/Quantize/Lock/Play/Cue panels remain visible.
Track time, pitch rate/range and absolute bar position remain visible.

Confirm `?.?` in the main BPM boxes, deck header chips, Grid headers and the
deck-preview footer on Browse, Sampler, Levels and every Settings sub-page.
Press and hold each numeric target with a native touch sequence: the real BPM
must appear only during the hold. Release, TouchCancel, drag-out, page hide and
window deactivation must remask immediately. Do not treat a direct button call
or mouse-only click as touch validation.

Combined 204px panel verified at 1024×600: Grid tab (994,70); deck 1
Earlier/Later (892/976,146), Set (934,194), BPM −/+ (892/976,242).
Deck 2 uses y=314/362/410. Touch targets and both-deck dragging passed.
The Grid panel and waveforms retain their geometry when the cue drawer opens.

Touch mode transition regression: banks share a stacked layout so overlapping
visibility notifications cannot add their heights or move the header. Cycle
all six modes repeatedly on both decks, including a held arrow press and
release; verify one advance per press, stable header/waveform geometry, and no
flash when entering or leaving Memory/Hot Cues. Mode IDs remain unchanged. Verify both arrows and the read-only mode label.

For a frame-level check in the prepared virtual-controller instance, record
`capture_touch_cycle.py` with `ffmpeg -f x11grab -draw_mouse 0 -framerate 60
-video_size 1024x600 -i :99 -c:v ffv1 /tmp/touch-cycle.mkv` (Night theme).
Stop the recording after the script completes, then run
`python3 tests/padfx/check_touch_cycle_frames.py /tmp/touch-cycle.mkv` inside
the owned container (copy the checker there first). It fails if the header
leaves its verified y=456–497 border after the drawer appears. Keep the
recording and JSON report under the task's ignored `test-results/` directory.

## Play Grid, Key and FX controls

The side panel preserves FX=0, Key=1, Jump=2 and Grid=3.
Key reads `visual_key`, uses ±2 semitone native commands, `sync_key` for harmonic
Match and `reset_key` for file-key reset. FX retains deck routing, enable and
wet/dry Mix, with a scrollable area for the first effect's loaded parameters.
Beat period buttons and controller Beat left/right share the native period and
range metadata. Pad mode labels are 12px; arrow touch targets remain 48×44px.

Key uses two bordered deck sections with 11px deck labels and centered
18px current-key badges. The `−2 st` / `+2 st` labels clarify semitone steps.
Key −2/+2 use (878/966,165) and y=321; Match/Reset use
(878/966,215) and y=371. Buttons remain 44px high; sections have an 8px gap.
Compact FX selector is (922,116); routing 1/2 and activation share y=152
at x=858/922/986. Echo’s Beat buttons use x=858/922/986, y=204/236.
Feedback/Ping Pong/Send knobs are (1001,265/295/325), Quantize/Triplets
(986,356/388), and Mix/Super knobs (902/1000,424). The viewport is y=168..404;
Echo fits without scrolling. With a scrollbar, parameter controls shift left
by its 14px width; longer effect lists retain scrolling.
The side panel is 204px wide. The waveform area expands from 820px at 1024×600
to 1076px at 1280×720, keeping the controls anchored at the right edge.

### Available FX controls

On the FX page, review Echo (1/8–2 beats), Phaser (1/4–4 beats), and Trans
(all six periods). Unsupported periods must be absent. Scroll to parameter
buttons, then select another effect: its parameter list must start at the top.
Select Enigma Jet to verify that Super is absent when no native parameter is
linked. The native clear-control test verifies that routing, enable, parameters
and Mix/Super hide for an unloaded chain while the selector remains available. Repeat in Night and Day. Saved presets
with missing backends remain on disk but are omitted from touch and controller
selection. Native tests cover all 25 standard presets and missing-backend files.

The compact FX list omits internal `mix` / `dry_wet` parameter rows using
read-only `parameterN_is_mix` metadata. Unit Mix remains visible; saved native
values and Super linkage are preserved. Check all available Standard/Saved
presets and backend manifests, including Flanger and White Noise, after changes.

Keep `#BeatFX_ParameterList` top-aligned within its 236px viewport. At 1024×600,
Flanger's first knob center is `(1000,183)`, with subsequent rows 30px apart;
Mix stays at y=424. For scroll checks use the label area `(835,340)`, not a
knob (wheel events over knobs adjust values). Phaser's Stereo row must remain
reachable and selecting Echo must restore the top of the parameter list.


### Touchable performance-pad regression

From a freshly started owned instance, run `python3 tests/e2e/check_touch_drawer.py`
and `python3 tests/e2e/check_performance_pads.py`. These cover all six pages on
both decks, forward/reverse wrap, and the actual pad hit areas/release highlight.
`node tests/padfx/test_touch_pads.cjs` checks the selected actions, saved-bank
snapshots and shared MIDI/touch ownership. Native `PadFxEditorTest` covers real
multitouch/cancellation; `PadFxRoutingTest` verifies touch reaches audio DSP.

`bash tests/padfx/test-live-midi-audio.sh` also runs `check_midi_pages.py` through
real virtual-PortMidi messages. It asserts both decks' Loop Shift/release,
FX 1/2 selection, FX 2 persistence after Shift release, all three Jump banks,
and touch Next continuing from the MIDI-selected FX 2 page. Screenshot/result
artifacts are saved under the owned instance's `test-results/` directory.


Queue feedback: no selection and unsupported views show an in-skin explanation;
success shows the added-track and pending-queue counts. **View Queue** `(598,56)`
uses `[AutoDJ],show_queue` to open Auto DJ directly, including an empty queue.
Saved playlists appear under **Folders → Playlists** when any exist; Rekordbox
playlists remain under their source. Open the playlist, then tap **Queue All**
to append its displayed tracks. Clear search to include the full playlist.
The desktop regression also seeds a saved playlist and queues both its entries.

### Settings preview badges

On General, Library, Device, Audio, System and Info, verify the bottom key and
BPM badges have matching 20px heights, centered 11px text and no clipping.
Check Traditional and Camelot key notation, shifted-key highlighting, and
Day/Night. PAD FX continues to use the whole page without a deck footer.

### Compact overview time ruler

The deck overview minute ruler is a separate 8px row with 6px labels. Verify both
decks at 1024×600 in RGB, FILT and 3 BAND, Night and Day. The 38px waveform
height, cue labels, time mapping and seek targets remain unchanged.

### System dashboard, clock and restart

See the [System/Info control map](../.agents/skills/bitedj-ui/references/settings.md#g-system-and-info-dashboard)
and [OS behavior](INFRASTRUCTURE.md#touch-datetime-boot-clocks-and-restart).
Verify Night and Day at 1024×600: all four Info cards, local date and tap hint,
output status, SSH service control and deck footer must remain visible. Tap Local Time; check
Region/City selectors, timezone preview including day rollover, calendar selection,
Hour/Minute controls, automatic sync hiding the manual fields,
Cancel, and disabled Apply when the date/time service is unavailable.

Tap **SSH REMOTE ACCESS · ENABLE / DISABLE** at `(763,486)`. The fullscreen
panel must report the current `ssh.service` state and expose Enable/Disable/Back
as 48px-or-larger targets centered near `y=436/496/556`. Native tests must route
Enable to `sudo -n systemctl enable --now ssh.service`, Disable to
`sudo -n systemctl disable --now ssh.service`, and reboot to
`sudo -n systemctl reboot` for the non-root appliance user. The container has no
SSH unit and should show a readable `not-found` state without enabling either
mutation button. Repeat the Info page and panel at 1280×720/1.20; expected
physical centers are recorded in the control map.

Native `SystemDialogsTest` uses temporary fake system executables to verify
manual/automatic clock commands, SSH enable/disable, privileged restart,
partial failures and confirmation/cancellation.
The clock-card regression sends `QTest::touchEvent` press/release events to the
displayed time label inside `WSystemInfo`, verifies the editor opens, then taps
Cancel and verifies the dashboard returns. Direct `click()` calls bypass this
touch-routing bug and are insufficient coverage.
`BootSettingsTest` uses a temporary boot file to verify preservation, reset,
backup, invalid/unsupported configurations and stale-write rejection. Neither
test modifies host time, boot settings or system power.

On the owned desktop, verify System → Power → Restart BiteDJ relaunches
the branch binary and retains settings. Check Restart system and Power off
confirmation/cancel. A container without systemd must report a visible failure
for an OS request. Hardware application of clock values and physical reboot
require a separately authorized Raspberry Pi test; container success does not
certify overclock stability.

The overclock dialog tests also click +/- to change 2000/750/6 to 2100/700/5,
save, reopen, restore firmware defaults, verify the original recovery copy,
and cancel the restart confirmation. Draft cancellation and external edits are
checked independently. The reader fixture supplies a simulated Pi model and a
temporary boot file; production always reads the fixed OS paths. Optional
`BITEDJ_UI_CAPTURE_DIR` captures this test with scrot on an owned X display.
These harness captures use the default Qt style; published skin captures come
from the actual application. See the control map above for verified touch targets.

Timezone tests verify zone-before-sync ordering, timezone-only edits without a
manual clock write, failure short-circuiting, and selecting 29 February 2028 with
a manual time. Use the current System/Info control map for touch targets. Test
commands are temporary fakes; no host timezone or clock is changed.

Verified timezone preview examples: America/Sao_Paulo (UTC−03) and
Pacific/Kiritimati (UTC+14), including the following calendar day. Cancelling
returns to the unchanged system clock. Night/Day calendar headers and touch
controls were checked at 1024×600.

Linux clock mutations run through noninteractive sudo when the app is non-root.
Run `SystemDialogsTest.*` as an unprivileged user with the temporary fake sudo
and timedatectl commands to check that path. BootSettingsTest validates the
headless helper request hash, numeric bounds, stale snapshots and original backup.
The helper must reject non-root invocation and malformed requests before any GUI
initialization; an invalid `QT_QPA_PLATFORM` is useful for checking this boundary.

Auto Play assigns visible BiteDJ decks 1/2 to the left/right crossfader sides
before starting. An inherited center assignment must never choose hidden decks
3/4. Hidden-deck playback prevents startup; other skins retain their routing.
The desktop fixture starts deck 1 centered to cover the physical-Pi regression.

On the Auto DJ queue itself, the compact toolbar shows **Remove** `(598,56)`,
**Move Up** `(683,56)`, and **Move Down** `(780,56)` instead of queue-add actions.
These use `[AutoDJ],remove_selected`, `move_up`, and `move_down`; `queue_view`
tracks native view visibility. Select a pending entry, then move or remove it.
Selection follows the moved entry; moves at either end do nothing. These actions
work with Auto Play on or off and use the existing playlist model, including its
next-track reload signal. Removing entries preserves the source playlist and file.
The desktop regression checks entry IDs/order, duplicates, boundaries, live
playback, and deleting the final pending entry.

### Large removable libraries

Drive enumeration on Linux reads the process mount table without probing the
filesystem. Settings, Browse and Rekordbox share `/media`, `/run/media` and
`/mnt` roots. Folder expansion and child-directory checks run asynchronously;
the expand arrow may appear briefly while its directory is checked. Existing
touch targets and saved controls are unchanged.

Folder rows arrive in batches of up to 100 with at most four batches queued.
Changing folders discards stale batches. Verify a 1,000-track synthetic folder,
rapid folder changes, BPM/key sorting without implicit track imports, and
Rekordbox playlist selection after a large export. A noisy drive or kernel
read/reset errors require stopping media tests; application changes cannot
repair unreadable hardware.


## 0.0.8 DDJ-400 jog and sync regression

On both loaded decks, test Shift + jog in FX, Key and Jump: position searches
quickly in either direction and the grid stays unchanged. In Grid, the same
input translates beat lines without seeking. Switch panels while holding Shift.
In General, compare Vinyl Brake Off, Short (1.8s) and Long (3.6s) with repeatable
throws; paused decks coast to rest and playing decks return to playback.
Switch to CDJ during a throw: scratch stops and normal jog bends pitch.
Enable then disable Beat Sync: tempo remains matched, but subsequent pitch/jog
changes are independent. This intentionally avoids a sudden tempo reset.
Fast mapping regressions cover MIDI routing; the native shipped-mapping test
measures Short/Long duration, and EngineSyncTest verifies independent pitch.

Beat Sync ON makes the pressed deck the tempo source and enables the other
deck as follower. OFF releases both decks while preserving their adjusted BPM.
After a manual jog nudge, phase offset must persist without snapping back.

For a repeatable live MIDI check on Linux, create a test preset using
`python3 tests/controllers/prepare_live_probe.py test-results/live-probe`.
Launch with `--settings-path` pointing to isolated settings and the two generated
test tracks. `BITEDJ_SETTINGS_PATH` is not an application settings-path option.
Build `tests/controllers/send_alsa_midi.c` with `cc -o send-midi ... -lasound`,
discover the application's input client/port using `aconnect -l`, then run
`python3 tests/controllers/check_live_probe.py LOG ./send-midi CLIENT PORT`.
The test-only probe logs native state; the runner sends actual ALSA MIDI through
the shipped XML callbacks. It verifies search/grid separation, Off/Short/Long
coast duration, CDJ behavior and Sync direction/phase release on both decks.
Physical gesture timing, LED appearance and speaker quality remain manual checks.

## Source recording indicator

Use two synthetic tracks in the owned 1024×600 instance. With decks on different
USB drives, record to each drive in turn: only the matching source badge turns
red. With both decks using the same recording USB, both badges turn red. Record
to a third drive or local directory: neither source badge lights, and the
Settings tab's top-right dot appears. Load a track from the recording drive,
then unload it, and verify the indicator moves between source and fallback.
Stop while playback continues: all recording dots must clear. Compare source
text and deck-label pixels across states to verify the reserved slot prevents
movement. Repeat Day/Night and check the finalized take. The former red `>>`
title marker must be absent. See the [Play control map](../.agents/skills/bitedj-ui/references/play.md#recording-indicator-beside-source)
for geometry and control mapping. On Pi, use USB Record/Stop and wait for
notifications to clear before top-bar navigation.

### Training line frame timing

Training phase rendering uses the shared waveform frame tick and one interpolated
audio playback position per deck for both the whole beat and its fraction. Do not
combine coarse `playposition` with independently sampled `beat_distance`. Hidden
phase widgets skip render work. Control IDs and geometry are unchanged.

For animation regression, record both synthetic decks playing from the beginning.
Use 120 fps X11 capture of the phase region to sample the 60 fps render clock,
retain timestamps (`-fps_mode passthrough`), and inspect both rows. Track red
markers modulo the four-beat spacing, treating disappearance beneath the fixed
white playhead as occlusion rather than a missing frame. Check capture timestamp
gaps separately. Do not run builds or E2E beside a performance capture.

Build .3 completes a bounded 160px-high repaint inside the shared render tick,
instead of queuing a repaint of the whole waveform lane. Its 30-second desktop
capture contained 3,600 video samples and 1,800 distinct phase positions per deck;
median/99th-percentile update intervals were 17ms, maximum 25ms at the capture's
8.33ms sampling resolution. No >26ms hold or >4.5px jump was detected. This does
not establish physical Pi display cadence.

Preserve the original capture timing when exporting video. The earlier stacked
comparison merged independent input timelines into an irregular cadence; even
fixed 60 fps downsampling of a 120 fps capture can introduce repeated positions.
The .3 delivery retains every source frame/timestamp without motion interpolation.
