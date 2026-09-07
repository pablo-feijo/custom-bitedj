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
