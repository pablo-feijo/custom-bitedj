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
- **Top Tab Bar (`topbar.xml`)**: `y=0..60`
  - `PLAY` (Overview): `x=100, y=30`
  - `BROWSE` (Library): `x=300, y=30`
  - `SAMPLER`: `x=500, y=30`
  - `LEVELS`: `x=700, y=30`
  - `SETTINGS`: `x=900..970, y=30`
- **Settings Sub-Tab Bar**: `y=60..112`
  - `GENERAL`: `x=100, y=85`
  - `LIBRARY`: `x=300, y=85`
  - `DEVICE`: `x=500, y=85`
  - `AUDIO`: `x=700, y=85`
  - `SYSTEM`: `x=900, y=85`
- **Settings Columns (`settings.xml`)**: Split into two 512px columns: Left `x=0..512`, Right `x=512..1024`.
  - Row height: 52px each, starting at `y=100` (`y_center = 124 + (row_index * 52)`).
  - Row centers:
    - Row 0 (`y=124`): `CROSSFADER` / `VINYL BRAKE`
    - Row 1 (`y=176`): `KEY` / `WAVE`
    - Row 2 (`y=228`): `DECK 1` / `APPLY WAVEFORM EQ`
    - Row 3 (`y=280`): `DECK 2` / `EQ MODE`
    - Row 4 (`y=332`): `JOG` / `CLEAR`
    - Row 5 (`y=384`): `HOT CUE` / `PLAYED`
  - Segmented 2-button group (`168f` width): Left segment center `x=876`, Right segment center `x=960`.
  - Levels page Master EQ buttons: `FLAT (x=845, y=240)`, `MODE (x=940, y=240)`.
- **Precision Pixel Coordinate Scanner (Python standard lib, no external dependencies)**:
  Convert screenshot to PPM (`ffmpeg -i screen.png screen.ppm`) and parse raw RGB bytes to find the exact bounding box of any colored element or text baseline before clicking.

### 3. Balanced Spacing & Clearance Calculation
- When resolving cramped UI items (e.g., text hugging a header), do not guess small increments (+2px or +4px).
- Measure both available margins: `gap_above` and `gap_below`.
- The balanced target position is `(gap_above + gap_below) / 2`.
- Verify both the element's own QSS (`margin-top`, `padding-left`) and its container's layout alignment (`qproperty-layoutAlignment`).
