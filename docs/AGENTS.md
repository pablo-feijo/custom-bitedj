# AI Agent Instructions

Hello! If you are an AI assistant or autonomous agent (like Antigravity, Claude, or GitHub Copilot) working on this codebase, **read this document before making changes.**

BiteDJ is a highly-customized fork of Mixxx, specifically engineered to run as a headless **Raspberry Pi OS Appliance** using the **Sway/Wayland compositor** and a multi-touch screen.

## Branch and Test Isolation — Required for Every New Task

- Before edits, inspect `git status`, branches, and `git worktree list`.
- Start each new implementation task on a new `codex/<topic>` feature branch
  from the intended semver integration branch, in a **separate worktree**.
  Never perform feature work directly on the semver branch or switch a checkout
  that contains another task's work. Reuse the feature worktree for follow-ups
  to the same task; do not create another branch for every message.
- Record the intended merge target in the task checklist. The user-designated
  target for the PiFlex first batch is `codex/v0.0.7`. Do not substitute `main`
  or an older release branch. If the target does not yet exist, preserve the
  agreed base and document the future target; do not invent its starting state.
- Keep application version numbers at the feature branch's inherited version
  until preparing the semver release. Then apply the synchronization protocol below.
- Every worktree must have its own `build-linux/`, `dist-linux/`, test settings,
  results, and GUI container. Never share writable build/install/config directories
  between branches. A shared compiler cache and immutable Docker image are fine.
- Use `run-gui-test.sh` and `gui-test-settings.sh`: they derive a worktree-specific
  container name, assign free host ports by default, and refuse to replace a
  container labeled for another worktree/branch. Explicit names/ports may be set
  with `BITEDJ_TEST_INSTANCE`, `BITEDJ_TEST_WEB_PORT`, `BITEDJ_TEST_AUDIO_PORT`,
  and `BITEDJ_TEST_VNC_PORT`.
- Read the printed endpoints or `docker port`; **never assume port 6080 or the
  legacy `bitedj-gui-test-instance` belongs to this task**. Source
  `gui-test-settings.sh` and run `verify_test_instance_owner` before manual
  test operations. Use `$CONTAINER_NAME` in commands.
- For stopping Mixxx in an owned instance, use
  `docker exec "$CONTAINER_NAME" pkill -9 mixxx`. Never use `killall`, global
  container cleanup, or remove another task's test instance.
- Existing fixtures under the original `test-config/` are not automatically
  copied. New instances use `test-config/<instance>/`; explicitly choose any
  settings migration. Optional USB test media should be read-only and selected
  for that task; no developer-specific volume is mounted automatically.
- Build/test the feature branch and leave its VNC available when requested.
  Report its exact branch, instance, endpoints, validation results, and merge
  target. Merge or deploy to hardware only when requested.

See [GUI_TESTING.md](GUI_TESTING.md) for reproducible commands. Historical
fixed-name commands later in this document describe the old single-instance
setup; replace their target with the verified owned `$CONTAINER_NAME`.

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
   - Topbar tabs are at `y=20` with 200px step (`PLAY=100`, `BROWSE=300`, `SAMPLER=500`, `LEVELS=700`, `SETTINGS=950`).
   - Settings sub-tabs are at `y=60`: GENERAL `x=73`, LIBRARY `x=219`, PAD FX `x=366` (third), DEVICE `x=512`, AUDIO `x=658`, SYSTEM `x=805`, INFO `x=951`. Keep WidgetStack indices stable; only reorder named tab buttons.
   - General settings: mixer/playback on the left; waveform/display and cleanup on the right. Use the verified option coordinates and control/value mappings in [AGENTS.md, section D](../AGENTS.md#d-settings---general-options-x73-y60). Standard rows are 52px; Track Load is 58px. Do not reuse former row positions.
   - Levels page Master EQ buttons: `FLAT (x=845, y=240)`, `MODE (x=940, y=240)`.
3. **Spacing & Margin Sizing**:
   - When fixing cramped margins, measure both opposing gaps (`gap_above` and `gap_below`) and target the visual midpoint `(gap_above + gap_below) / 2` on the first iteration rather than testing tentative 2px increments.

### E. Keep UI Guides in Sync
Every UI option addition, removal, rename, move or resize must update the root
[UI guide](../AGENTS.md) and [GUI testing guide](GUI_TESTING.md) in the same
commit. Record option order, measured 1024×600 coordinates and control/value
mappings; update affected automation and controller docs. Preserve keys, enum
values and saved page indices for layout-only changes. Verify labels, touch
clearance, padding and footer visibility in the owned VNC instance; include
Day/Night checks when styling changes. Prefer a canonical mapping link over
stale duplicate coordinates.

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

For the 0.0.7 working release, pi-gen also uses `codex/v0.0.7`. Commit changes
there first, publish that branch, then commit the parent `mixxx-pi-gen` gitlink.
Keep `.gitmodules` URL/branch valid so a recursive clone resolves the pinned
commit. The flasher derives IMG_NAME from the pinned config; use
`BITEDJ_IMAGE_DATE=YYYY-MM-DD` to select a build from a different day.
Do not copy UI resources into pi-gen: it consumes the matching parent ARM64
`dist-linux` build. Its sample mixxx.cfg is not installed on first boot.

## 5. Prevent Configuration Drift (Infrastructure as Code)
When resolving bugs on live hardware or applying hot-patches over SSH (e.g., editing `~/.config/sway/config` or modifying `gsettings` on the Pi), you **must immediately backport those changes to the local repository.** 
- Never leave a live Pi in a state that cannot be exactly reproduced by `./generate-pi-image.sh`.
- If you fix a system issue, commit the corresponding changes to the `mixxx-pi-gen` submodule (e.g., injecting the fix into `i3.conf` or `01-run.sh`) so the local build state remains the absolute source of truth.

## 6. Commit Message Convention

All new and amended commits must use Conventional Commits:
`<type>[optional scope]: <description>` (for example,
`fix(effects): publish programmatic enable changes to the audio engine`).
Use an appropriate type such as `feat`, `fix`, `docs`, `refactor`, `test`,
`build`, `ci`, `perf`, or `chore`. Use `!` and a `BREAKING CHANGE:` footer
when applicable. Keep each commit focused and include source attribution
in the body for adapted upstream work.

Before finishing a task, check the commits created by that task and amend
any nonconforming messages. Do not rewrite unrelated or already-published
history unless the user explicitly requests it.
