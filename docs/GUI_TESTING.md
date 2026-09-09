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
git fetch origin
git worktree add ../bitedj-my-feature -b codex/my-feature origin/codex/v0.0.7
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

Use the [synthetic fixture generator and procedure](../tests/rekordbox/README.md). Browse uses 44px rows: collapsed roots are Prepare `y=63`, Computer `107`, History `151`, Rekordbox `195`; its expanded fixture child is `239`. Each expanded child shifts subsequent rows by 44px. Add tracks with **Add to Prepare** in their context menu. In Prepare, use **Move Up**, **Move Down**, or **Remove**; verify order after restarting. Loading retains queue entries and never starts playback automatically.

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

### Docker cleanup and recovery

See [Docker maintenance](DOCKER_MAINTENANCE.md) for full prune, disk checks and
recovery after engine failure. A full prune removes stopped GUI instances and
may remove their unused image. Host `test-config/` remains; relaunch from the
correct worktree and use its freshly printed endpoints.

### Beat FX picker

At 1024×600, Play's FX tab is `(878,70)`, selector `(934,122)`, Deck 1/2
routing `(891,174)` / `(977,174)`, and activation `(934,226)`. The picker
stays inside `x=852..1015, y=100..599`. Its two effect columns have centers
`x=894,974`; seven row centers are `y=194,242,290,338,386,434,482`. Effect targets are
76px wide and 44px tall with 10px labels, with 4px gaps and outside padding.
Standard/Saved: `(894,119)` / `(974,119)`; Clear FX/Close: `(894,153)` /
`(974,153)`; Prev/Next: `(894,538)` / `(974,538)`. Header/footer targets
are 30px tall; the page counter is 9px at `(934,514)`. Leaving FX closes the picker, including a skin reload.

Standard order is row-major, entries 1–14, then 15–25, listed in
[Beat FX](BEAT_FX.md). Saved holds legacy/custom entries. Native persisted IDs
remain independent of labels. Page/section changes and Close/Escape must leave
the selected chain unchanged. Selection starts a standard chain Off; the
existing Effect1 `enabled` control activates all its occupied slots together.
The `parameterN_beat_period` alias uses periods in beats and converts rate-based
parameters without changing saved raw values.

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
Beat Loop replace the pad grid with a **read-only controller legend**. Use the
physical pads to perform the displayed actions. X still closes the drawer;
selecting a controller mode opens that deck again. Mode button release does
not close it. Other modes close only their own deck's visible drawer.

Runtime controls (not saved): `[PadFX],dN_mode` = 0 Hot Cue, 1 Pad FX,
2 Beat Jump, 3 Beat Loop, 4 Memory; `dN_shift` = 0/1; `dN_jump_bank` = 0/1/2 for
1/16, 1, 16 multipliers. The existing mapping shares jump sizes across decks.

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

Current header coordinates and control mappings are in the root
[touch drawer guide](../AGENTS.md#touch-drawer-navigation-and-padding-1024600).
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

Use the [canonical Browse coordinates](../AGENTS.md#b-browse--library-navigation).
With the synthetic `/music` Quick Link configured, tap Computer `(150,107)`,
Quick Links `(150,151)`, then Music `(200,195)`. Group labels must expand
without switching to a table; a folder's indentation cell expands its
subfolders without opening its tracks. Tap its label to open tracks, then
Folders `(980,62)` to return. Drag a long tree without opening any row.
Headers at `y=84..117` use 11px text with 8px padding; verify sorting at
header center `y=101`, compact rows at `y=129,151`, and footer visibility.
In Settings → Library, Preview enable is `(816,391)` and L width `(988,391)`.
Both are needed to show wide preview waveforms in a fresh test configuration.
Check Browse and the FX picker in Day and Night modes.

For the nested-folder swipe check, create synthetic empty directories before
starting the owned instance so its lazy directory cache sees them:

```sh
mkdir -p "test-music/Touch navigation/Nested"
for i in $(seq 1 16); do mkdir -p "test-music/Touch navigation/Nested/Folder $i"; done
```

After Computer and Quick Links, use expansion cells `(110,195)`, `(154,239)`
and `(198,283)` for Music, Touch navigation and Nested. Drag from `(300,459)`
to `(300,239)`; the tree must scroll and remain open. Restore the scroll and
tap Music's label to verify that a folder with children can still open tracks.

Compact FX actions: eraser = Clear FX, × = Close, left/right chevrons =
Prev/Next. Tooltips and accessible names retain the action labels. Standard
and Saved remain labeled tabs; the 9px page counter reads `1 / 2`.

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
