# Controller pad drawer task

- Active SemVer base and merge target: `codex/v0.0.7`.
- Fetched base commit: `559edc3def`.
- Feature branch: `codex/controller-pad-drawer`.
- Planned binary version: `0.0.7-codex-controller-pad-drawer.2` (not built).
- Worktree: `bitedj-controller-pad-drawer`.
- [x] Create an isolated worktree from the latest remote integration tip.
- [x] Follow DDJ-400 pad mode selection independently per deck.
- [x] Display configured Pad FX, Beat Jump and Beat Loop options in the cue drawer.
- [x] Test mode changes, release handling, both decks and GUI layout.
- [x] Version and verify any deliverable build artifacts.
- Merge only when requested.

Validation so far: Node mapping regression and existing Pad FX regression pass.
Development artifact version: `0.0.7-codex-controller-pad-drawer.2`.
ARM64 application build passed. Five native PadFxEditor/PadFxSettings tests
passed, including the drawer layout test. Simulated MIDI verified both decks,
mode releases, Shift, other-mode dismissal and X close/reopen at 1024×600.
Day/Night captures inspected and published in the UI gallery.
Binary `--version` verified: `0.0.7-codex-controller-pad-drawer.2`.
Instance: `bitedj-gui-3825487826`; noVNC `http://localhost:52285/vnc.html`,
audio port 52286, VNC port 52284. Physical controller validation remains separate.
No OS image is being produced by this task.

Integration note: the parallel `codex/ddj400-shift-fx-back` task introduces a
new two-column Beat FX picker and musical `_beat_period` alias. Its coordinates
must be verified after integration; they are not the current drawer branch UI.
Do not overwrite that task's work or reuse its new selector coordinates before merging.

Touch follow-up: header cycles Hot Cues → Memory → Beat Jump → Pad FX →
Beat Loop on both decks, including without a controller. Existing numeric mode
IDs preserved; Memory appended as 4. Full cycle and controller-to-touch handoff
verified at (530,467); all five selected native tests passed.
