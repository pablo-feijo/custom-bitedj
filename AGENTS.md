# AI Agent Instructions

Hello! If you are an AI assistant or autonomous agent (like Antigravity, Claude, or GitHub Copilot) working on this codebase, **read this document before making changes.**

BiteDJ is a highly-customized fork of Mixxx, specifically engineered to run as a headless **Raspberry Pi OS Appliance** using the **Sway/Wayland compositor** and a multi-touch screen.

## 1. Architectural Rules for Agents

### A. Do Not Use QDrag for Touchscreen Drag-and-Drop
Qt6 Wayland currently suffers from severe grab-serial desynchronization bugs when using standard `QDrag` APIs on multi-touch hardware. 
- If you are asked to fix or modify Drag-and-Drop, **do not** attempt to use `QMimeData` or `QDrag`. 
- We use a **Custom Overlay UI** (a floating `QLabel`) intercepting `mouseMoveEvent` and `mouseReleaseEvent` in `src/widget/wtracktableview.cpp`. Maintain this pattern.

### B. Do Not Modify UI Geometry Manually
Sway is a tiling window manager. The application runs natively in fullscreen, and dialogs (like Preferences) are meant to spawn in fullscreen as well.
- **Rule**: Never use `this->setGeometry()` or `this->resize()` in C++ dialog constructors.
- Always use `this->showFullScreen()` for dialogs to prevent the compositor from splitting the screen in half and breaking Qt's internal layouts.

### C. Do Not Append to the GUI Thread
BiteDJ runs on slow USB flash storage.
- Never execute database writes or log appends on the main GUI thread.
- Always utilize `FsHistoryWorker` or a dedicated `SCHED_BATCH` thread for I/O to prevent 3-second UI freezes.

### D. First-Time-Right GUI Automation & Debugging Protocol
When developing or testing in `bitedj-gui-test-instance`:
1. **Container Tool Constraints**:
   - Kill Mixxx with `pkill -9 mixxx` (never `killall`).
   - Screenshot with `DISPLAY=:99 scrot /tmp/file.png` (never `import`).
   - Crop images with `ffmpeg -y -i <in.png> -vf "crop=<w>:<h>:<x>:<y>" <out.png>`.
   - Never assume Python `PIL` or OpenCV are present in the testing environment.
2. **Deterministic UI Coordinate Targeting**:
   - Never guess pixel coordinates for `xdotool`. Calculate them from XML layout widths or scan the exact bounding box using standard library Python on PPM dumps (`ffmpeg -i in.png out.ppm`).
   - Topbar tabs are at `y=30` with 200px step (`PLAY=100`, `BROWSE=300`, `SAMPLER=500`, `LEVELS=700`, `SETTINGS=950`).
   - Settings rows are 52px each starting at `y=100` (`y_center = 124 + row_index * 52`: Row 0 `y=124`, Row 1 `y=176`, Row 2 `y=228`, Row 3 `y=280`, Row 4 `y=332`, Row 5 `y=384`).
   - Right-aligned segmented buttons (168px): Left segment center `x=876`, Right segment center `x=960`.
   - Levels page Master EQ buttons: `FLAT (x=845, y=240)`, `MODE (x=940, y=240)`.
3. **Spacing & Margin Sizing**:
   - When fixing cramped margins, measure both opposing gaps (`gap_above` and `gap_below`) and target the visual midpoint `(gap_above + gap_below) / 2` on the first iteration rather than testing tentative 2px increments.

## 2. Infrastructure & Build Workflows

If the user asks you to compile or test the application, use the scripts provided in the root directory:

- **Compiling for the Pi**: Run `./docker-build.sh --platform linux/arm64`. This uses a custom Docker container to cross-compile the binary into `dist-linux/`. Do not try to compile natively on a Mac or Windows machine using standard `CMake` unless you are explicitly building a local debug version.
- **Local GUI & Audio Testing**: Before deploying changes or building an OS image, run `./test-gui-automated.sh` to automatically verify the 1024x600 GUI, skin layouts, FX rack DSP, and live audio stream without touching hardware. Run `./run-gui-test.sh` for interactive testing via browser at `http://localhost:6080/` and live audio at `http://localhost:8000/`. Full details in `docs/GUI_TESTING.md`.
- **Hot-Deploying**: Use `./deploy-ssh.sh` to push a newly compiled ARM64 binary to a live Raspberry Pi over the network.
- **Flashing the OS**: The complete Raspberry Pi OS is generated using `./generate-pi-image.sh`, which leverages the `mixxx-pi-gen` submodule.

## 3. Important Context
Before attempting large refactors or upstream cherry-picking from `mixxxdj/mixxx`, review the following documents:
- `docs/DIFFS_FROM_BASE.md`: Master ledger of all C++ engine and OS changes vs upstream.
- `docs/INFRASTRUCTURE.md`: Explains Polkit permissions, Kernel realtime scheduling (`preempt=full`), and Docker dependencies.
- `docs/DDJ400_MAPPING.md`: Explains the custom Pioneer DDJ-400 Pad FX logic and Hardware UI interception.

## 4. Versioning Protocol
When preparing a new release or branch (e.g., `v0.0.4`), agents must explicitly synchronize the Semantic Version (semver) across the entire stack:
1. **Source Code**: Ensure `BITEDJ_VERSION` in `CMakeLists.txt` matches the target branch semver (e.g., `0.0.4`). This updates the `WVersionLabel` in the Settings UI automatically.
2. **OS Image output**: Ensure `IMG_NAME` in `mixxx-pi-gen/config` includes the semver suffix (e.g., `IMG_NAME="bitedj-pi-v0.0.4"`).
3. **Flashing Scripts**: Update `flash-sdcard.sh` dynamically or explicitly so `ZIP_FILE` and `IMG_FILE` point to the freshly versioned output targets.

## 5. Prevent Configuration Drift (Infrastructure as Code)
When resolving bugs on live hardware or applying hot-patches over SSH (e.g., editing `~/.config/sway/config` or modifying `gsettings` on the Pi), you **must immediately backport those changes to the local repository.** 
- Never leave a live Pi in a state that cannot be exactly reproduced by `./generate-pi-image.sh`.
- If you fix a system issue, commit the corresponding changes to the `mixxx-pi-gen` submodule (e.g., injecting the fix into `i3.conf` or `01-run.sh`) so the local build state remains the absolute source of truth.
# Agent Instructions

## UI Layout
- Overview Panel: We are intentionally keeping only the **FX** and **KEY** tabs for now. 
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

#### A. Top Tab Bar (`topbar.xml`): `y=0..60`
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

#### C. Settings Sub-Tab Bar: `y=60..112`
- `GENERAL`: `x=100, y=85`
- `LIBRARY`: `x=300, y=85`
- `DEVICE`: `x=500, y=85`
- `AUDIO`: `x=700, y=85`
- `SYSTEM`: `x=900, y=85`

#### D. Settings -> General Options (`x=100, y=85`)
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

#### E. Settings -> Library Options (`x=300, y=85`)
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
