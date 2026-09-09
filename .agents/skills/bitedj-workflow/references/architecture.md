# Architecture and attribution

[Codex setup](../../../../docs/CODEX.md). Read this guide when its task trigger applies.

Custom Bite DJ is an independent fork of [Team Deckshark’s BiteDJ](https://github.com/TeamDeckshark/bitedj), based on [Mixxx](https://github.com/mixxxdj/mixxx). It targets a headless Raspberry Pi OS appliance with Sway/Wayland and a multi-touch screen.

## Architectural rules

### Do Not Use QDrag for Touchscreen Drag-and-Drop

Qt6 Wayland currently suffers from severe grab-serial desynchronization bugs when using standard `QDrag` APIs on multi-touch hardware.
- If you are asked to fix or modify Drag-and-Drop, **do not** attempt to use `QMimeData` or `QDrag`.
- We use a **Custom Overlay UI** (a floating `QLabel`) intercepting `mouseMoveEvent` and `mouseReleaseEvent` in `src/widget/wtracktableview.cpp`. Maintain this pattern.

### Use in-skin notifications

`MixxxApplication::notify` automatically rejects modal dialogs in kiosk mode.
Use `Notifications::publish` for operational feedback; do not add QMessageBox
prompts. The notification strip covers the topbar and can be tapped to dismiss.

### Do Not Modify UI Geometry Manually

Sway is a tiling window manager. The application runs natively in fullscreen, and dialogs (like Preferences) are meant to spawn in fullscreen as well.
- **Rule**: Never use `this->setGeometry()` or `this->resize()` in C++ dialog constructors.
- The panel-contained [Beat FX picker](../../bitedj-ui/references/effects.md#panel-contained-beat-fx-picker)
  is an explicit native exception; preserve its bounded child layout.
- Always use `this->showFullScreen()` for other dialogs to prevent the compositor from splitting the screen in half and breaking Qt's internal layouts.

### Do Not Append to the GUI Thread

BiteDJ runs on slow USB flash storage.
- Never execute database writes or log appends on the main GUI thread.
- Always utilize `FsHistoryWorker` or a dedicated `SCHED_BATCH` thread for I/O to prevent 3-second UI freezes.

## UI Layout

- Overview Panel: Keep **FX**, **KEY**, **JUMP**, and **GRID** tabs. Beat-jump size and actions belong in JUMP, not the left waveform sidebar.
- This build targets two decks. Keep Deck 1/2 UI and controller routing; do not
  import four-deck layouts or controls from the reviewed fork.
- Do not attempt to add `PADS` or `CFX` tabs back to the native `WidgetStack` in `effects.xml`.
- Effect times (e.g., Roll lengths 1/8, 1/4, 1/2, 1) are mapped natively through the skin's Beats parameter grid (which appears automatically for `_units == 1` Beats-typed parameters).

## Attribution and Licensing

- Main-program fork changes remain GPL-2.0-or-later; skin adaptations retain
  GPLv3. Add scoped Custom Bite DJ copyright notices while retaining upstream
  credits and the standard license text. Keep LICENSE, COPYING and NOTICE.md
  consistent and packaged together; do not introduce a blanket proprietary or
  noncommercial license. Follow [the licensing policy](../../../../docs/LICENSING.md).

- Identify this project as **Custom Bite DJ**, an independent fork of Team
  Deckshark's BiteDJ, based on Mixxx. Link upstream and distinguish inherited
  work, adaptations and local changes; do not imply endorsement or upstream support.
- Preserve copyright notices, author credits and license texts. Keep source
  identifiers, paths and historical documents accurate; never globally replace
  “BiteDJ” or “Mixxx” inside notices, code keys or third-party material.
- Follow [LICENSING.md](../../../../docs/LICENSING.md) when changing attribution or preparing
  distribution. The program and skin have distinct license notices. Document
  upstream paths/revisions and dated modifications when adapting material.
- Recheck component terms before importing code or artwork. Credits alone are
  not permission, and a documentation review is not a complete release audit.

## Refactors and upstream imports

Before attempting large refactors or upstream cherry-picking from `mixxxdj/mixxx`, review the following documents:
- [Upstream differences](../../../../docs/DIFFS_FROM_BASE.md): Master ledger of all C++ engine and OS changes vs upstream.
- [Infrastructure](../../../../docs/INFRASTRUCTURE.md): Explains Polkit permissions, Kernel realtime scheduling (`preempt=full`), and Docker dependencies.
- [DDJ-400 mapping](../../../../docs/DDJ400_MAPPING.md): Explains the custom Pioneer DDJ-400 Pad FX logic and Hardware UI interception.
