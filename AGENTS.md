# Agent Instructions

## Commit Messages
- Use Conventional Commits for every commit: `type(scope): description` (scope is optional).
- Use appropriate types such as `feat`, `fix`, `docs`, `refactor`, `test`, `build`, or `chore`.
- Before finishing, check commits created for the current task and amend any nonconforming messages. Do not rewrite unrelated history.
Read [docs/AGENTS.md](docs/AGENTS.md) before making changes. It contains the
architecture, versioning, branch isolation, testing and Conventional Commits rules.

## Required Task Workflow

- Start every new task on a new `codex/<topic>` feature branch in a separate Git
  worktree, based on the agreed semver integration branch. Reuse that worktree
  for follow-ups and record the intended merge target in the task checklist.
- Keep build outputs, installed binaries, settings and test containers independent
  per worktree. Never switch or overwrite another task's checkout or VNC instance.
- Use `run-gui-test.sh`; select free ports automatically or set
  `BITEDJ_TEST_INSTANCE`, `BITEDJ_TEST_WEB_PORT`, `BITEDJ_TEST_AUDIO_PORT`, and
  `BITEDJ_TEST_VNC_PORT`. Read the printed endpoints, not assumed port 6080.
- Before manual test commands below, run `source ./gui-test-settings.sh` and
  `verify_test_instance_owner`. `$CONTAINER_NAME` must identify this task's owned
  instance. See [GUI testing](docs/GUI_TESTING.md) for the full recipe.
- Use Conventional Commits for every new or amended commit. Merge into the
  agreed semver branch later when requested; synchronize versions for the release.

## Pad FX Architecture
- System settings own Pad FX defaults, saved overrides and reset commands.
  Skins only place the optional editor; never embed presets or effect logic in XML.
- Keep the Settings editor usable at 1024×600: eight pad selectors, one visible
  assignment editor, minimum 44px touch controls, no extra overview tabs.
  Hide the Settings deck footer only on PAD FX; preserve outer and card padding.

## UI Change Documentation

- Whenever adding, removing, renaming, moving or resizing a UI option, update
  its option order, verified coordinates and control/value mappings in this guide
  and [docs/GUI_TESTING.md](docs/GUI_TESTING.md) in the same commit.
- Update affected automation coordinates, controller documentation and screenshots
  when their targets or behavior change. Prefer links to the canonical mapping
  above instead of maintaining contradictory copies; never reuse stale coordinates.
- Preserve control keys, enum values and persisted WidgetStack indices for layout-only
  changes. If behavior changes, document the new mapping and any settings migration.
- Verify the changed page in the owned VNC instance at 1024×600, including labels,
  touch targets, padding and footer visibility; check Day/Night when styles change.

## UI Layout
- Overview Panel: Keep **FX**, **KEY**, and **JUMP** tabs. Beat-jump size and actions belong in JUMP, not the left waveform sidebar.
- This build targets two decks. Keep Deck 1/2 UI and controller routing; do not
  import four-deck layouts or controls from the reviewed fork.
- Do not attempt to add `PADS` or `CFX` tabs back to the native `WidgetStack` in `effects.xml`.
- Effect times (e.g., Roll lengths 1/8, 1/4, 1/2, 1) are mapped natively through the skin's Beats parameter grid (which appears automatically for `_units == 1` Beats-typed parameters).

## First-Time-Right GUI Debugging & Interaction Strategies

To ensure changes and automated UI tests succeed on the first attempt without trial-and-error:

### 1. Container Tooling & Environment Constraints
- **Process Management**: Always use `pkill -9 mixxx` to terminate Mixxx in the verified `$CONTAINER_NAME` instance. Never call `killall` (not installed in container).
- **Restart Recipe**:
  ```bash
  docker exec "$CONTAINER_NAME" pkill -9 mixxx && sleep 1 && \
  docker exec -d "$CONTAINER_NAME" bash -c "DISPLAY=:99 QT_AUTO_SCREEN_SCALE_FACTOR=0 QT_ENABLE_HIGHDPI_SCALING=0 QT_SCALE_FACTOR=1.0 BITEDJ_SETTINGS_PATH=/root/.mixxx /dist-linux/bin/mixxx /music/BiteDJ_Test_Groove_128BPM.wav /music/BiteDJ_Test_Techno_124BPM.wav --resourcePath /dist-linux/share/mixxx/ --full-screen --style Fusion"
  ```
- **Screenshot Capture**: Always use `DISPLAY=:99 scrot /tmp/screen.png` inside the container, then `docker cp` to host. Never use ImageMagick `import` (not installed).
- **Image Cropping**: Always use `ffmpeg -y -i <in.png> -vf "crop=<w>:<h>:<x>:<y>" <out.png>`. Do not assume Python `PIL` or OpenCV are installed.
- **Hot-Reloading Skins**: Remember `/dist-linux` is bind-mounted `:ro` from the host repository. Edit files in `res/skins/BiteDJ/` and copy them to `dist-linux/share/mixxx/skins/BiteDJ/` on the host to immediately update the container before restarting Mixxx.

### 2. Zero-Guessing Precision Coordinate System (1024x600)
Never guess pixel coordinates for `xdotool` clicks. Use the exact layout geometry or scan via standard library Python:

The top bar is now 40px (previously 60px). The legacy coordinates below remain
reference points: main-tab centers are now y=20; settings sub-tabs and top-anchored
content move up 20px. Expanding overview lanes may reposition centered controls,
so remeasure those from a current screenshot before clicking.

#### A. Top Tab Bar (`topbar.xml`): `y=0..40`
- `PLAY` (Overview): `x=100, y=30`
- `BROWSE` (Library): `x=300, y=30`
- `SAMPLER`: `x=500, y=30`
- `LEVELS`: `x=700, y=30`
- `SETTINGS`: `x=900..970, y=30` (recommended: `x=950, y=30`)

#### B. Browse / Library Navigation
- **Breadcrumb Back Button (toggles sidebar tree / library view)**: `x=963, y=79`
- **Sidebar Tree Rows (when sidebar is visible)**:
  - `COMPUTER`: `x=100, y=107`
  - `QUICK LINKS`: `x=100, y=135` (expands to show Music)
  - `MUSIC`: `x=100, y=142` (under Quick Links; loads `/music` directory table)
  - `REMOVABLE DEVICES`: `x=100, y=170`
  - `HISTORY`: `x=100, y=205`
  - *(Note: iTunes, Traktor, Rhythmbox, and Banshee are disabled by default to avoid clutter)*
- **Reliable Keyboard Navigation Flow to load `/music`**:
  ```bash
  # 1. Ensure in Browse tab
  docker exec "$CONTAINER_NAME" bash -c "DISPLAY=:99 xdotool mousemove 300 30 click 1"
  # 2. Click Back button to reveal sidebar tree if currently in table view
  docker exec "$CONTAINER_NAME" bash -c "DISPLAY=:99 xdotool mousemove 963 79 click 1"
  # 3. Direct click on MUSIC under Quick Links (once expanded)
  docker exec "$CONTAINER_NAME" bash -c "DISPLAY=:99 xdotool mousemove 100 142 click 1"
  # Alternatively, keyboard sequence from sidebar focus:
  docker exec "$CONTAINER_NAME" bash -c "DISPLAY=:99 xdotool key Up Up Right Down Right Down Return"
  ```
- **Track Table Geometry (when music loaded)**:
  - Table Header: `y=195`
  - Track Row 0: `y=218`
  - Track Row 1: `y=240` (row spacing = +22px in Compact, +38px in Detail)

#### C. Settings Sub-Tab Bar: `y=40..80`
Visible order and verified button centers at 1024×600:
- `GENERAL`: `x=73, y=60`
- `LIBRARY`: `x=219, y=60`
- `PAD FX`: `x=366, y=60` (third option)
- `DEVICE`: `x=512, y=60`
- `AUDIO`: `x=658, y=60`
- `SYSTEM`: `x=805, y=60`
- `INFO`: `x=951, y=60`

Button order is independent of the persisted WidgetStack page indices. Move
buttons by their named triggers; keep stack order stable to preserve saved tabs.
PAD FX fills the remaining screen and hides the deck footer; other tabs retain it.

#### D. Settings -> General Options (`x=73, y=60`)

Verified at 1024×600. Left: mixer and playback. Right: waveform/display and
cleanup. Standard row centers are `104 + 52 * row`; Track Load uses a 58px
row with center `y=470`. All rows fit above the deck footer at `y=520`.

| Column | Option | y | Button centers x (left to right) | Control mapping |
| --- | --- | --- | --- | --- |
| Left | Crossfader | 104 | Off 374, On 458 | `[BiteDJ],crossfader_enabled`: Off=0, On=1 |
| Left | Deck 1 | 156 | A 360, None 416, B 472 | `[Channel1],orientation`: A=0, None=1, B=2 |
| Left | Deck 2 | 208 | A 360, None 416, B 472 | `[Channel2],orientation`: A=0, None=1, B=2 |
| Left | EQ Mode | 260 | EQ 374, ISO 458 | `[BiteDJ],eq_mode`: EQ=0, ISO=1 |
| Left | Jog | 312 | Vinyl 374, CDJ 458 | `[BiteDJ],vinyl_mode`: Vinyl=1, CDJ=0 |
| Left | Vinyl Brake | 364 | Off 360, Short 416, Long 472 | `[BiteDJ],vinyl_brake`: Off=0, Short=1.8, Long=3.6 |
| Left | Hot Cue | 416 | Ungated 374, Gated 458 | `[Controls],HotcueActivatePlays`: Ungated=1, Gated=0 |
| Left | Track Load | 470 | Lock 290, Fader 350, Stop 410, Live 470 | `[BiteDJ],track_load_policy`: Lock=0, Fader=3, Stop=2, Live=1 |
| Right | Wave | 104 | RGB 872, Filt 928, 3 Band 984 | `[Waveform],waveform_type`: RGB=17, Filt=19, 3 Band=25 |
| Right | Apply Waveform EQ | 156 | On 886, Off 970 | `[Waveform],apply_eq_to_waveform`: On=1, Off=0 |
| Right | Palette | 208 | BiteDJ 886, Amber 970 | `[BiteDJ],waveform_palette`: BiteDJ=0, Amber=1 |
| Right | Key | 260 | Camelot 886, Trad 970 | `[Library],key_notation`: Camelot=3, Trad=4 |
| Right | Grid | 312 | Compact 886, Detail 970 | `[Library],grid_layout`: Compact=0, Detail=1 |
| Right | Clear | 364 | Cache 872, Cues 928, Meta 984 | Cache: `[Library],clear_cached_waveforms`; Cues: `[Library],clear_cue_overrides`; Meta: `[Library],clear_meta_overrides` |
| Right | Played | 416 | Reset 932 | Reset: `[Library],reset_played_tracks` |

#### E. Settings -> Library Options (`x=219, y=60`)
Configures visible columns and column widths (`OFF | XS | S | M | L`).
- **Left Column (`x=0..512`)**:
  - Column buttons at: `OFF (x=296)`, `XS (x=346)`, `S (x=386)`, `M (x=426)`, `L (x=466)`
  - Row 0 (`y=104`): `#`
  - Row 1 (`y=156`): `TITLE`
  - Row 2 (`y=208`): `ARTIST`
  - Row 3 (`y=260`): `ALBUM`
  - Row 4 (`y=312`): `BPM`
  - Row 5 (`y=364`): `KEY`
  - Row 6 (`y=416`): `TIME`
- **Right Column (`x=512..1024`)**:
  - Column buttons at: `OFF (x=796)`, `XS (x=846)`, `S (x=886)`, `M (x=926)`, `L (x=966)`
  - Row 0 (`y=104`): `GENRE`
  - Row 1 (`y=156`): `YEAR`
  - Row 2 (`y=208`): `COLOR`
  - Row 3 (`y=260`): `RATING`
  - Row 4 (`y=312`): `PLAYED`
  - Row 5 (`y=364`): `COMMENT`
  - Row 6 (`y=416`): `PREVIEW` (e.g. click `x=966, y=416` for `L` size)

#### F. Levels Page (`x=700, y=30`)
- Master EQ buttons: `FLAT (x=845, y=240)`, `MODE (x=940, y=240)`.

#### G. Precision Pixel Coordinate Scanner (Python standard lib, no external dependencies)
Convert screenshot to PPM (`ffmpeg -i screen.png screen.ppm`) and parse raw RGB bytes to find the exact bounding box of any colored element or text baseline before clicking.

### 3. Balanced Spacing & Clearance Calculation
- When resolving cramped UI items (e.g., text hugging a header), do not guess small increments (+2px or +4px).
- Measure both available margins: `gap_above` and `gap_below`.
- The balanced target position is `(gap_above + gap_below) / 2`.
- Verify both the element's own QSS (`margin-top`, `padding-left`) and its container's layout alignment (`qproperty-layoutAlignment`).

#### H. Overview Right Panel (1024x600)
- Tab centers: `FX (878,90)`, `KEY (934,90)`, `JUMP (990,90)`.
- KEY: Deck 1 `-2 (891,178)`, `+2 (977,178)`, `RESET (934,230)`;
  Deck 2 uses the same x coordinates at `y=326` and `y=378`.
- JUMP: Deck 1 halve/double at `(891,178)` / `(977,178)`,
  backward/forward at `(891,230)` / `(977,230)`;
  Deck 2 uses `y=326` and `y=378`.
- FX: selector `(934,142)`, deck routing `(891,194)` / `(977,194)`,
  activation `(934,246)`. Parameter-grid positions depend on the selected effect.
- Wait for display-mode notifications to clear before clicking the main tabs;
  the notification temporarily covers the top bar.

## Deck interaction additions
- Linked waveform zoom is below the two JUMP sections. Use native zoom controls
  and the waveform factory's synchronization; do not step both synchronized decks
  separately, which would apply each action twice.
- Deck time modes use persisted `[Skin],deck1_time_mode` and `deck2_time_mode`.
- For touch library loading, begin with a horizontal move to distinguish a drag
  from vertical scrolling. Only actual visible deck regions accept that drag;
  Escape cancels it. Highlight geometry must match the release target geometry.
- Overview waveform height must match its visible container (currently 38px).
  Keep the 12px minute ruler in a separate row; do not crop a double-height widget.
  Check RGB, FILT and 3 BAND after changing overview rendering. Stacked rendering
  uses bottom-origin image coordinates and must not get the symmetric translation.
- Current compact Browse table: breadcrumb y=40..72, header y=72..94,
  fixture row centers y=104 and y=126. Top-menu centers now use y=20.
