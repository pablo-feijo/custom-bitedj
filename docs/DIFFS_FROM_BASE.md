# Differences from Upstream Mixxx & Cherry-Picking Ledger

<!-- Modified for Custom Bite DJ on 2026-09-09: clarify fork identity and attribution. -->

This document tracks all divergences from upstream Mixxx, specifically formatted to assist in evaluating upstream commits for cherry-picking. It is divided into two distinct layers:
1. **Base BiteDJ (v1.0-1)**: [Team Deckshark’s BiteDJ](https://github.com/TeamDeckshark/bitedj), the foundational fork that optimized Mixxx for standalone hardware, focusing on audio path resilience, USB stability, and SQLite threading.
2. **Custom BiteDJ (v0.0.6)**: Our tailored branch built on top of Base BiteDJ, specifically engineered for native Wayland integration, screen rotation persistence, DRM hardware cursor workarounds, touchscreen drag-and-drop, and club-ready DDJ-400 mappings.

## Selected PiFlex adaptations

Working branch `codex/xsploit-readme-feature-map`, intended target `codex/v0.0.7`.
Source: xsploit/bitedj `4c1dfec590f98851159fe7a64e3348e8aad306a5`.
See [test coverage](TESTING.md) and [licensing](LICENSING.md).

- Explicit engine publication for programmatic effect enable/disable.
- Persisted browser column ordering and text size, with model/proxy identity
  preserved during sorting. Managed widths and Wayland track dragging retained.
- Rekordbox page-chain bounds/cycle checks and independent DAT/EXT analysis import
  that warns on optional-data failure while retaining audio loading.
- Worktree-owned GUI instances, independent ports/config/results, and reusable
  branch/semver/Conventional Commits rules for agents.

These are selected adaptations of xsploit/bitedj at `4c1dfec590`, not a fork merge.

---

## 1. Custom BiteDJ (v0.0.6 - Display Persistence, DRM Cursor Fixes, & UI Polish)

These features were engineered to stabilize standalone hardware rotation, preserve cursor visibility, and polish the touchscreen layout.

### A. Screen Rotation Persistence across Sessions & Reboots (`src/preferences/systemsettings.cpp`)
- **Live Sway Transformation**: When 0° or 180° rotation is triggered, `SystemSettings::applyScreenRotation()` auto-discovers the active `SWAYSOCK` across `/run/user/<uid>` and commands Sway (`output * transform <degrees>`).
- **Persistent Config Sync**: Scans and rewrites `output * transform` in `~/.config/sway/config`, `/home/pi/.config/sway/config`, and all user directories, followed by a POSIX `sync()` to ensure changes survive unexpected power cycles.
- **Startup Enforcement**: `SystemSettings` re-applies the saved rotation during constructor initialization on boot.

### B. DRM Hardware Cursor Workaround (`WLR_NO_HARDWARE_CURSORS=1`)
- **Problem**: Raspberry Pi KMS DRM drivers (`vc4`/`v3d`) do not support hardware plane rotation for cursors on transformed outputs, causing the mouse cursor to disappear at 180°.
- **Fix**: Injected `WLR_NO_HARDWARE_CURSORS=1` into `/etc/environment`, user profiles, and image build scripts (`mixxx-pi-gen`), falling back to software cursor rendering with programmatic Sway cursor nudges.

### C. System Settings & Power Layout (`res/skins/BiteDJ/system_tab.xml`, `style.qss`)
- Standardized button widths (`114f`) and spacing (`4f`) across System and Advanced rows for visual symmetry.
- Relocated the Power / Shutdown button to the Advanced row with in-place confirmation display.

### D. Sampler Row Polish (`res/skins/BiteDJ/sampler.xml`, `style.qss`)
- Matched `STOP ALL` button height and styling to adjacent row controls with balanced text padding.
- Corrected label clipping on the `SOURCE` text element.

### E. Deck Header & Waveform Controls Polish (`res/skins/BiteDJ/style.qss`, `waveform.xml`)
- **Neutral Idle Play/Cue**: Neutralized idle play/cue button colors with dedicated `play.svg` / `pause.svg` icons.
- **Restored Chip Geometry**: Restored original compact dimensions for `#DeckKey`, `#DeckHeaderBPM`, and `#DeckCueChip` while preserving high-contrast cyan (`#38bdf8`) typography.
- **Cue to Title Spacing**: Tightened padding between `#DeckCueChip` and `#DeckTitle` (`margin-right: 2px;`, `padding-left: 0px;`).
- **Waveform Key Note Alignment**: Added `padding-left: 6px;` and `margin-top: 12px;` to `#WaveformInfo_Key` for clean clearance under deck badges and alignment with the controls below.

### F. DDJ-400 Shift + Filter Mapping (`res/controllers/Pioneer-DDJ-400-script.js`)
- Mapped `Shift + Filter` knobs directly to the Beat FX Super parameter for dual-hand Super and Mix sweeps during live performances.

---

## 2. Custom BiteDJ (v0.0.5 - Wayland, Touch, & Audio Optimizations)

These features were engineered specifically to make BiteDJ operate flawlessly on a touchscreen Raspberry Pi running the Sway compositor.

### A. Custom Drag-and-Drop Subsystem (`src/widget/wtracktableview.cpp`)
- **Base Mixxx**: Uses `QDrag` and `QMimeData` which requests the OS compositor to handle drag avatars and drop targets.
- **Custom BiteDJ**: Intercepts `QMouseEvent` in `WTrackTableView` to create a custom floating `QLabel` containing the Track Title and Artist. Manual widget bounding box detection (`this->rect().contains()`) is used in `mouseReleaseEvent` to route the `TrackPointer` perfectly, bypassing Qt6 Wayland grab-serial touchscreen deadlocks.
- *Cherry-Pick Action*: Do NOT cherry-pick upstream DND fixes in `WTrackTableView` unless they explicitly support Wayland Touch.

### B. Native Fullscreen Preferences (`src/preferences/dialog/dlgpreferences.cpp`)
- **Base Mixxx**: `DlgPreferences` attempts to manually compute its geometry using `resize()` and `setGeometry()` relative to hardcoded screen margins.
- **Custom BiteDJ**: Manual geometry overrides have been purged. `this->showFullScreen()` is called directly in the constructor to seamlessly integrate with Sway tiling without splitting the screen in half.
- *Cherry-Pick Action*: Reject upstream geometry calculation changes for dialogs.

### C. Advanced Skin Controls & UI Hooks
- **Touch-to-Cycle Time (`src/widget/wnumberpos.cpp`)**: Track duration elements in `deck.xml` are encased in invisible `PushButton` overlays that reliably toggle `[Controls],ShowDurationRemaining` upon a fat-finger touch.
- **Crossfader Toggle**: Added `[BiteDJ],crossfader_enabled` in `settings.xml`. When toggled off, `mixxxmainwindow.cpp` dynamically re-routes `[ChannelX],orientation` to the Center, neutralizing physical hardware crossfaders.
- **Preferences Button**: Added a dedicated "Preferences" trigger inside the custom UI settings menu (`[Master],show_preferences`) to invoke the native Qt preferences dialog.

### D. Audio Device Whitelisting & Naming (`src/preferences/audiodevicesettings.cpp`)
- **Base Mixxx**: Enumerates raw ALSA string nodes directly into the GUI output.
- **Custom BiteDJ**: Implements a strict UI filter that hides dangerous hardware nodes, explicitly permitting `pipewire` and `sysdefault`. To provide a seamless DJ experience, the C++ engine intercepts these nodes and forcibly renames them to **"PipeWire / Bluetooth"** in the custom skin before rendering, completely preventing PortAudio from panicking the Raspberry Pi kernel.

### E. Pioneer DDJ-400 Controller Mapping (`res/controllers/Pioneer-DDJ-400-script.js`)
- Extensive custom XML/JS modifications for instant doubles and Pad FX integration (see `DDJ400_MAPPING.md` for full details).
- *Cherry-Pick Action*: Isolate upstream mapping additions and cherry-pick specific improvements (like LED feedback updates) without touching BiteDJ's Pad FX or Release FX logic.

---

## 3. Base BiteDJ (v1.0-1 - Core Infrastructure)

These foundational features were implemented by the original BiteDJ fork to survive the harsh reality of standalone USB environments.

### A. Audio Path Resilience & Threading (`src/preferences/audiodevicesettings.cpp`)
- **Device Watchdog**: PortAudio natively fails to report USB audio interface removal. Base BiteDJ implements a custom stream liveness poller. If the USB soundcard loses power, the app tears it down and automatically retries re-enumeration every 3 seconds, seamlessly bypassing ALSA `hw:X,Y` stale index bugs.
- **USB Blink Survival**: Upstream Mixxx instantly ejects a track if a USB drive read returns empty. `CachingReaderWorker` in Base BiteDJ re-opens the file in place and provides a 5-second grace window, saving the set if a USB stick is accidentally bumped.
- **Strict Thread Priorities**: Audio callback and disk I/O are elevated to true Linux `SCHED_FIFO` priorities. Non-critical waveform analysis is demoted to `SCHED_BATCH` (nice 19) to ensure rendering never preempts audio.
- *Cherry-Pick Action*: Merge audio routing changes carefully, preserving BiteDJ's retry loops and device liveness polling.

### B. EQ & Isolator Math
- **Global `eq_mode`**: Added a global preference (`[BiteDJ],eq_mode`) dictating how EQs cut. "Isolator" (the default) reaches absolute silence at zero, while "EQ" bottoms out at a finite shelf.
- **Linear Taper**: Deck EQ parameters were switched to a linear taper so MIDI `64` aligns perfectly with the physical center detent of controller knobs.

### C. Database & USB History
- **Threaded History**: Upstream appends history logs on the GUI thread (which can freeze the UI for up to 3 seconds on slow USBs). Base BiteDJ pushes all history reads/writes to a dedicated `FsHistoryWorker` thread.
- **WAL Mode & Lock Splitting**: The SQLite library database utilizes `WAL` (Write-Ahead Logging). Rekordbox and Serato library scans are split into explicit read/write phases so they don't hold global write locks that freeze the UI while a DJ is trying to browse a playlist.

---

## 4. Upstream Cherry-Pick Checklist

When evaluating new Mixxx releases (e.g., 2.5, 2.6), prioritize reviewing the following upstream areas for potential cherry-picks:
- [ ] **Library & Database Optimizations**: Upstream improvements to SQLite queries or track parsing algorithms.
- [ ] **BPM & Key Detection Algorithms**: Enhancements to Queen Mary DSP or Rubberband processing.
- [ ] **New Audio Formats / Decoders**: Flac, Opus, or AAC codec support updates.
- [ ] **Controller Scripts**: Hardware-specific XML/JS updates for non-DDJ-400 controllers that you wish to support.

### D. FX Panel Touch UI Adjustments (`res/skins/BiteDJ/effects.xml`)
- **Base Mixxx**: Relies on a hardware MIDI controller (SuperKnob) to drive effects, hiding key parameters when loaded.
- **Custom BiteDJ**: Added permanent `MIX` and `SUPER` (SuperKnob) control knobs directly to the bottom of the FX panel in the `BiteDJ` skin to allow standalone touchscreen users to control effect depth, width, and wet/dry mix without needing a physical controller attached.

## v0.0.6-waveform Updates
- Reverted progress-bar waveform rendering to ensure waveforms always stretch to fill the cell entirely.
- Fixed fake drag-and-drop label to hover 10px above the touch point, preventing finger occlusion.
- Implemented true black background and LOAD text state for empty cells instead of buggy fillPath.
## v0.0.6 (RGB Stacked Waveform Fixes)
- **C++ Factory Registration**: Explicitly mapped the `25` waveform enum to the string `"3-Band (GLSL)"` in `WaveformWidgetFactory`, fixing a bug where Mixxx would silently reject the user's setting and fallback to RGB.
- **Preview Button Connections**: Ensured the global `[Waveform] waveform_type` ControlObject is instantiated *before* `WTrackTableView` creates `PreviewButtonDelegate`s. This resolves the bug where track library waveforms were permanently frozen on the startup setting.
- **Overview Stack Sync**: Created a dedicated `WaveformOverviewType` property in the backend to ensure Deck `WOverview` waveforms respond natively to the 3-Band setting change.
- **WaveformRendererFiltered Track Colors**: Updated `WaveformRendererFiltered` to correctly source dynamic RGB track colors (`m_rgbLowColor`, etc.) for `mode == 2`, fixing a major rendering bug where 3-Band stacked waveforms were drawing black due to an undefined generic skin color fallback.

## Pending v0.0.7: configurable Pad FX

`codex/rekordbox-padfx-display` adds system-owned assignments/reset commands,
private native effect lanes and a compact full-height Settings editor. DDJ-400
normal/Shift pads use the new mapping instead of swapping the main Beat FX slot.
See [Pad FX validation](PAD_FX_TESTING.md). Implementation plans are local execution records under ignored `tasks/`.


### 2026-09-09 — Preview loading and rendering

Browse summary-only I/O now runs on a single background pool with bounded caches;
painting uses a published-object lookup without filesystem canonicalization or
metadata-import waits. Cached images include data identity and completion, and
partial summaries use full-track coordinates. Shared waveform-type mapping and
palette updates cover Browse/Play/deck overviews. Palette changes rebuild only
scrolling waveform widgets. Deck summaries reset incremental/scaled images and
use the summary's own width before engine duration controls catch up.
Native rendering/delegate/cache regressions and paused GPU E2E checks accompany
these fixes. The controller pad drawer dependency from `5a84488ae3` also gains
explicit previous/next touch navigation and balanced padding in this task.

### Jog filter preferences (0.0.7)

`RateControl` reads `[Controls] JogWheelFilterLength` (6, clamped 1–64).
Deck service preferences Apply publishes the length atomically; audio callbacks
reset their own preallocated Rotary buffer when the setting changes. Rotary
capacity is 64 while its general default remains 50; length changes clear stale
samples and reset the cursor. No allocation or configuration lookup occurs in
the audio callback. DDJ-400 behavior is documented in [the mapping](DDJ400_MAPPING.md#jog-alignment-release-and-service-smoothing).
