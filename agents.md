# Agent Instructions

## Commit Messages
- Use Conventional Commits for every commit: `type(scope): description` (scope is optional).
- Use appropriate types such as `feat`, `fix`, `docs`, `refactor`, `test`, `build`, or `chore`.
- Before finishing, check commits created for the current task and amend any nonconforming messages. Do not rewrite unrelated history.

## UI Layout
- Overview Panel: Keep **FX**, **KEY**, and **JUMP** tabs. Beat-jump size and actions belong in JUMP, not the left waveform sidebar.
- Do not attempt to add `PADS` or `CFX` tabs back to the native `WidgetStack` in `effects.xml`.
- Effect times (e.g., Roll lengths 1/8, 1/4, 1/2, 1) are mapped natively through the skin's Beats parameter grid (which appears automatically for `_units == 1` Beats-typed parameters).

## First-Time-Right GUI Debugging & Interaction Strategies

To ensure changes and automated UI tests succeed on the first attempt without trial-and-error:

### 1. Container Tooling & Environment Constraints
- **Process Management**: Always use `pkill -9 mixxx` to terminate Mixxx in `bitedj-gui-test-instance`. Never call `killall` (not installed in container).
- **Restart Recipe**:
  ```bash
  docker exec bitedj-gui-test-instance pkill -9 mixxx && sleep 1 && \
  docker exec -d bitedj-gui-test-instance bash -c "DISPLAY=:99 QT_AUTO_SCREEN_SCALE_FACTOR=0 QT_ENABLE_HIGHDPI_SCALING=0 QT_SCALE_FACTOR=1.0 BITEDJ_SETTINGS_PATH=/root/.mixxx /dist-linux/bin/mixxx /music/BiteDJ_Test_Groove_128BPM.wav /music/BiteDJ_Test_Techno_124BPM.wav --resourcePath /dist-linux/share/mixxx/ --full-screen --style Fusion"
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
  docker exec bitedj-gui-test-instance bash -c "DISPLAY=:99 xdotool mousemove 300 30 click 1"
  # 2. Click Back button to reveal sidebar tree if currently in table view
  docker exec bitedj-gui-test-instance bash -c "DISPLAY=:99 xdotool mousemove 963 79 click 1"
  # 3. Direct click on MUSIC under Quick Links (once expanded)
  docker exec bitedj-gui-test-instance bash -c "DISPLAY=:99 xdotool mousemove 100 142 click 1"
  # Alternatively, keyboard sequence from sidebar focus:
  docker exec bitedj-gui-test-instance bash -c "DISPLAY=:99 xdotool key Up Up Right Down Right Down Return"
  ```
- **Track Table Geometry (when music loaded)**:
  - Table Header: `y=195`
  - Track Row 0: `y=218`
  - Track Row 1: `y=240` (row spacing = +22px in Compact, +38px in Detail)

#### C. Settings Sub-Tab Bar: `y=60..100`
- `GENERAL`: `x=85, y=80`
- `LIBRARY`: `x=255, y=80`
- `DEVICE`: `x=425, y=80`
- `AUDIO`: `x=595, y=80`
- `SYSTEM`: `x=765, y=80`
- `INFO`: `x=937, y=80`

#### D. Settings -> General Options (`x=85, y=80`)
Split into two 512px columns. Row height: 52px each, starting at `y=100` (`y_center = 124 + (row_index * 52)`).
- **Left Column (`x=0..512`)**:
  - Row 0 (`y=124`) `CROSSFADER`: `OFF (x=366)`, `ON (x=446)`
  - Row 1 (`y=176`) `KEY`: `CAMELOT (x=366)`, `TRAD (x=446)`
  - Row 2 (`y=228`) `DECK 1`: `A (x=350)`, `NONE (x=406)`, `B (x=462)`
  - Row 3 (`y=280`) `DECK 2`: `A (x=350)`, `NONE (x=406)`, `B (x=462)`
  - Row 4 (`y=332`) `JOG`: `VINYL (x=366)`, `CDJ (x=446)`
  - Row 5 (`y=384`) `HOT CUE`: `UNGATED (x=366)`, `GATED (x=446)`
  - Row 6 (`y=436`) `GRID`: `COMPACT (x=366)`, `DETAIL (x=446)`
- **Right Column (`x=512..1024`)**:
  - Row 0 (`y=124`) `VINYL BRAKE`: `OFF (x=852)`, `SHORT (x=908)`, `LONG (x=964)`
  - Row 1 (`y=176`) `WAVE`: `RGB (x=852)`, `FILT (x=908)`, `STACK (x=964)`
  - Row 2 (`y=228`) `APPLY WAVEFORM EQ`: `ON (x=866)`, `OFF (x=946)`
  - Row 3 (`y=280`) `EQ MODE`: `EQ (x=866)`, `ISO (x=946)`
  - Row 4 (`y=332`) `CLEAR`: `CACHE (x=852)`, `CUES (x=908)`, `META (x=964)`
  - Row 5 (`y=384`) `PLAYED`: `RESET (x=910)`

#### E. Settings -> Library Options (`x=255, y=80`)
Configures visible columns and column widths (`OFF | XS | S | M | L`).
- **Left Column (`x=0..512`)**:
  - Column buttons at: `OFF (x=296)`, `XS (x=346)`, `S (x=386)`, `M (x=426)`, `L (x=466)`
  - Row 0 (`y=124`): `#`
  - Row 1 (`y=176`): `TITLE`
  - Row 2 (`y=228`): `ARTIST`
  - Row 3 (`y=280`): `ALBUM`
  - Row 4 (`y=332`): `BPM`
  - Row 5 (`y=384`): `KEY`
  - Row 6 (`y=436`): `TIME`
- **Right Column (`x=512..1024`)**:
  - Column buttons at: `OFF (x=796)`, `XS (x=846)`, `S (x=886)`, `M (x=926)`, `L (x=966)`
  - Row 0 (`y=124`): `GENRE`
  - Row 1 (`y=176`): `YEAR`
  - Row 2 (`y=228`): `COLOR`
  - Row 3 (`y=280`): `RATING`
  - Row 4 (`y=332`): `PLAYED`
  - Row 5 (`y=384`): `COMMENT`
  - Row 6 (`y=436`): `PREVIEW` (e.g. click `x=966, y=436` for `L` size)

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
