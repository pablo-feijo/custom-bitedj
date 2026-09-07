# Differences from Upstream Mixxx & Cherry-Picking Ledger

This document tracks all divergences from upstream Mixxx, specifically formatted to assist in evaluating upstream commits for cherry-picking. It is divided into two distinct layers:
1. **Base BiteDJ (v1.0-1)**: The foundational fork that optimized Mixxx for standalone hardware, focusing on audio path resilience, USB stability, and SQLite threading.
2. **Custom BiteDJ (v0.0.3)**: Our tailored branch built on top of Base BiteDJ, specifically engineered for native Wayland integration, touchscreen drag-and-drop, and club-ready DDJ-400 mappings.

---

## 1. Custom BiteDJ (v0.0.5 - Wayland, Touch, & Audio Optimizations)

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

## 2. Base BiteDJ (v1.0-1 - Core Infrastructure)

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

## 3. Upstream Cherry-Pick Checklist

When evaluating new Mixxx releases (e.g., 2.5, 2.6), prioritize reviewing the following upstream areas for potential cherry-picks:
- [ ] **Library & Database Optimizations**: Upstream improvements to SQLite queries or track parsing algorithms.
- [ ] **BPM & Key Detection Algorithms**: Enhancements to Queen Mary DSP or Rubberband processing.
- [ ] **New Audio Formats / Decoders**: Flac, Opus, or AAC codec support updates.
- [ ] **Controller Scripts**: Hardware-specific XML/JS updates for non-DDJ-400 controllers that you wish to support.

### D. FX Panel Touch UI Adjustments (`res/skins/BiteDJ/effects.xml`)
- **Base Mixxx**: Relies on a hardware MIDI controller (SuperKnob) to drive effects, hiding key parameters when loaded.
- **Custom BiteDJ**: Added permanent `MIX` and `SUPER` (SuperKnob) control knobs directly to the bottom of the FX panel in the `BiteDJ` skin to allow standalone touchscreen users to control effect depth, width, and wet/dry mix without needing a physical controller attached.
