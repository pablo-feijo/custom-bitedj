# Agent Instructions

<!-- Modified for Custom Bite DJ on 2026-09-09: clarify fork identity and attribution. -->

## Active Unreleased Branch and Binary Versions
- Current active unreleased base: `origin/codex/v0.0.7`, target SemVer `0.0.7`.
- Follow [the branch and version guide](docs/BRANCH_VERSIONING.md). Fetch the active remote tip before each new task; create an isolated `codex/<topic>` worktree from that tip. Reuse the task worktree for follow-ups.
- Track the base branch, resolved commit, feature branch, worktree, binary version and merge target in the task checklist. When the user advances the active release, update this record, `docs/AGENTS.md` and the guide's active-release table/history together.
- Every work-branch binary must embed `MAJOR.MINOR.PATCH-<full-branch-slug>.<build-number>` using `BITEDJ_VERSION` and `BITEDJ_VERSION_PRERELEASE`; for example `0.0.7-codex-controller-pad-drawer.1`. Increment the build number for new deliverable builds. Keep the upstream Mixxx version independent.
- Synchronize full binary/package/archive/image versions, record the original branch and source commit as provenance, and verify the rebuilt binary's reported version before delivery. Never relabel an old binary. Unsuffixed versions are reserved for final releases.

## Commit Messages

- Integrate each task into a semver branch with a squash merge: one Conventional Commit for the complete task, without bringing its intermediate commits or a merge commit into the semver history.
- Rewrite previously published semver commits only when explicitly requested. Preserve unrelated history and a local backup ref, and push rewritten history with an explicit expected-value force-with-lease.
- Use Conventional Commits for every commit: `type(scope): description` (scope is optional).
- Use appropriate types such as `feat`, `fix`, `docs`, `refactor`, `test`, `build`, or `chore`.
- Before finishing, check commits created for the current task and amend any nonconforming messages. Do not rewrite unrelated history.
Read [docs/AGENTS.md](docs/AGENTS.md) before making changes. It contains the
architecture, versioning, branch isolation, testing and Conventional Commits rules.

## Required Task Workflow

- Start every new task on a new `codex/<topic>` feature branch in a separate Git
  worktree, based on the latest agreed semver integration branch (currently
  `codex/v0.0.7`). Fetch the matching remote ref and inspect the local integration
  tip before branching; preserve local integration commits. Reuse that worktree
  for follow-ups and record the intended merge target in the task checklist.
- Keep build outputs, installed binaries, settings and test containers independent
  per worktree. Never switch or overwrite another task's checkout or VNC instance.
- Use `scripts/test/run-gui-test.sh`; select free ports automatically or set
  `BITEDJ_TEST_INSTANCE`, `BITEDJ_TEST_WEB_PORT`, `BITEDJ_TEST_AUDIO_PORT`, and
  `BITEDJ_TEST_VNC_PORT`. Read the printed endpoints, not assumed port 6080.
- Before manual test commands below, run `source ./scripts/test/gui-test-settings.sh` and
  `verify_test_instance_owner`. `$CONTAINER_NAME` must identify this task's owned
  instance. See [GUI testing](docs/GUI_TESTING.md) for the full recipe.
- Use Conventional Commits for every new or amended commit. Merge into the
  agreed semver branch later when requested; synchronize versions for the release.

## Post-Merge CI

- Follow the compiled-output cache strategy in [TESTING.md](docs/TESTING.md#reuse-compiled-outputs-for-asset-changes).
  Asset-only changes should reuse exact compatible CI binaries and refresh assets;
  always rerun tests. Invalidate on compiled input/toolchain changes, preserve
  original binary provenance, and retain full versioned rebuilds for deliverables.

- After every merge and push to the active SemVer branch, check CI for the exact
  published commit and follow all expected jobs through completion, including
  desktop E2E. Follow [the post-merge procedure](docs/TESTING.md#post-merge-ci-check-and-repair).
- Investigate and fix failures as part of the task; do not stop at reporting red
  CI. Validate the repair, publish it when integration is authorized, and check
  the resulting SemVer commit again until all expected checks pass. Keep pending,
  cancelled, missing or externally blocked checks explicitly unresolved.

## Branch Hygiene

- After authorized integration and publication, clean up fully merged task
  branches locally and on the user's remote using the
  [canonical branch cleanup procedure](docs/AGENTS.md#branch-cleanup-after-integration).
- Keep semver/release branches, `main`, the remote default branch, tags and all
  unmerged work. Preserve active worktrees and branches backing live GUI instances;
  report deferred cleanup rather than disrupting another task.
- Verify ancestry and remote reachability, including pinned submodule commits,
  before deleting branches. Keep generated files in ignored runtime directories;
  do not combine branch cleanup with destructive filesystem cleanup.

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

## Published UI Screenshots

- UI changes must refresh every affected image in `docs/images/ui/0.0.7/`
  and its caption in [the UI gallery](docs/UI_SCREENSHOTS.md) in the same commit.
  This includes visible labels, controls, layout, styling and waveform rendering.
- Follow [the capture procedure](docs/GUI_TESTING.md#documentation-screenshots).
  Use an owned 1024×600 instance and synthetic fixtures; inspect each final image.
- Link the affected gallery section from the corresponding UI changelog entry.
  Keep README previews and gallery links valid when moving or renaming images.
- Curated documentation images are committed assets, explicitly requested for
  the README/docs. Raw captures and other test-run results remain ignored under
  `test-results/`; do not commit those.
- Refresh the current unreleased version's gallery in place. When starting a new
  release, retain released images and use a new version directory and gallery so
  historical changelog links continue to show their release's UI.

## Fork Attribution

Identify this repository as Custom Bite DJ, an independent fork of Team
Deckshark's BiteDJ, based on Mixxx. Preserve upstream copyright and license
notices; follow the [attribution rules](docs/AGENTS.md#attribution-and-licensing)
and [licensing notes](docs/LICENSING.md). Never claim upstream endorsement or
replace technical identifiers and historical credits as a branding cleanup.

## Repository Organization

- Follow [the canonical repository layout](docs/REPOSITORY_LAYOUT.md) for every new file.
  Put BiteDJ helpers in `scripts/build/`, `scripts/deploy/`, `scripts/test/` or
  `scripts/legacy/`; Docker recipes in `docker/`; guides and plans in `docs/`;
  integration fixtures in `tests/`. Keep existing upstream utilities in `tools/`.
- Do not accumulate scripts or scratch files at the root. Root additions require
  a tool-discovery need or deliberate project entry point, explained in the change.
- Keep generated output in ignored, worktree-local runtime directories. Curated
  documentation images belong under `docs/images/`, never at the root.
- For every move, update all callers, Docker paths, CI/config references, tests,
  README/docs and agent instructions. Search tracked and hidden files, preserve
  executable permissions, and verify path resolution from outside the repo.
- Update the canonical map when adding a category; link to it instead of copying it.

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
At 1024×600, folder rows are 44px high. With Computer and Quick Links
expanded: Prepare `(150,63)`, Computer `(150,107)`, Quick Links `(150,151)`,
Music `(200,195)`, Removable Devices `(180,239)`, History `(150,283)`.
Rows below expanded children move by 44px per child; remeasure other trees.
Tap a grouping row to expand/collapse. For folders with tracks and subfolders,
tap the 44px indentation cell to expand; tap the label to open tracks.
Arrow-cell centers are `x=22` for roots, `66` for their children and `110`
for grandchildren. Drag vertically to scroll without selecting.

The track table's **Folders** button `(980,62)` restores navigation using
`[Sidebar],sidebar_visible`; opening a folder sets it to 0. Table headers
use 11px text with 8px padding, at `y=84..117`. Compact track centers are
`y=129,151` (22px spacing). Column visibility, sort and size controls retain
their existing values. See [GUI testing](docs/GUI_TESTING.md#browse-touch-navigation).

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
| Right | Phrases | 104 | Toggle 788 | `[BiteDJ],show_phrases`: Off=0, On=1 (default); native two-state toggle |
| Right | Wave | 104 | RGB 872, Filt 928, 3 Band 984 | `[Waveform],waveform_type`: RGB=17, Filt=19, 3 Band=25 |
| Right | Apply Waveform EQ | 156 | On 886, Off 970 | `[Waveform],apply_eq_to_waveform`: On=1, Off=0 |
| Right | Palette | 208 | BiteDJ 886, Amber 970 | `[BiteDJ],waveform_palette`: BiteDJ=0, Amber=1 |
| Right | Key | 260 | Camelot 886, Trad 970 | `[Library],key_notation`: Camelot=3, Trad=4 |
| Right | Grid | 312 | Compact 886, Detail 970 | `[Library],grid_layout`: Compact=0, Detail=1 |
| Right | Clear | 364 | Cache 872, Cues 928, Meta 984 | Cache: `[Library],clear_cached_waveforms`; Cues: `[Library],clear_cue_overrides`; Meta: `[Library],clear_meta_overrides` |
| Right | Played | 416 | Reset 932 | Reset: `[Library],reset_played_tracks` |
| Right | Return to Play | 470 | Off 886, On 970 | `[BiteDJ],return_to_play`: Off=0 (default), On=1; successful main-deck Browse loads only |

Browse root rows: Prepare `y=52`, Computer `y=75`, History `y=99`, Rekordbox `y=121` (expanded fixture child `y=143`). Overview labels use hot-cue letters and memory numbers; full names remain in the Play waveform. Phrase strips are 10px with 8px text; the overview ruler is 9px with 7px text.

#### E. Settings -> Library Options (`x=219, y=60`)
Configures column visibility (independent ON/OFF toggle) and widths (`XS | S | M | L`).
Verified against the documentation capture at 1024×600, UI source `8f87aa338f`:
- **Left Column**: ON/OFF `x=304`; XS `356`, S `396`, M `436`, L `476`.
- **Right Column**: ON/OFF `x=820`; XS `868`, S `908`, M `948`, L `988`.
- Row centers are `y=102 + 48 * row`:

| Row | y | Left | Right |
| --- | --- | --- | --- |
| 0 | 102 | # | Genre |
| 1 | 150 | Title | Year |
| 2 | 198 | Artist | Color |
| 3 | 246 | Album | Rating |
| 4 | 294 | BPM | Played |
| 5 | 342 | Key | Comment |
| 6 | 390 | Time | Preview |

Visibility uses `[Library],column_visible_<column>` (Off=0, On=1); width
uses `[Library],column_weight_<column>` (XS=1, S=2, M=3, L=4). Selecting a width does not enable an
Off column. See [the current Library screenshot](docs/UI_SCREENSHOTS.md#settings-library).

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
- Tab centers: `FX (878,70)`, `KEY (934,70)`, `JUMP (990,70)`.
- KEY: Deck 1 `-2 (891,178)`, `+2 (977,178)`, `RESET (934,230)`;
  Deck 2 uses the same x coordinates at `y=326` and `y=378`.
- JUMP: Deck 1 halve/double at `(891,178)` / `(977,178)`,
  backward/forward at `(891,230)` / `(977,230)`;
  Deck 2 uses `y=326` and `y=378`.
- FX: selector `(934,122)`, deck routing `(891,174)` / `(977,174)`,
  activation `(934,226)`. Parameter-grid positions depend on the selected effect.
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

General Settings typography: labels 12px, segment/action text 11px; button
geometry and the coordinate mappings above are unchanged. This leaves clearance
for “3 Band” on the 1024×600 display.

## Reproducible Test Assets

Keep test generators, synthetic fixture definitions, reusable scripts and test
procedures in Git. Put generated exports, audio captures, screenshots, logs,
benchmark snapshots, caches and test reports in ignored `test-results/` (or
other ignored runtime directories). Do not commit test-run results. Record
instance ownership and regenerate assets instead of copying personal music.

Compact overview cue labels retain the cue color as a small badge with contrasting text. Verify both hot-cue letters and memory numbers in Day/Night, with phrases enabled and disabled. Fixtures include distinct cue colors to make regressions visible.

Bottom-preview cue priority: use 2px colored marker lines with a contrasting border, painted above the countdown watermark. Keep cue letters/numbers and phrase labels at 8px. Verify hot cues and memories remain distinct with phrases On/Off and in Day/Night.

The main `cue_point` is shown as an orange **CUE** marker in both bottom previews, matching Play (`#ff6000`). It remains visible when the playhead is exactly on the cue. Verify this separately from hot-cue letters and memory-cue numbers, with phrases On/Off.

At overlapping positions, the orange main **CUE** line and label paint last, above hot cues and memory cues. Keep the CUE label unabridged; cue metadata and existing edit targets are unchanged. Test exact overlaps with a hot cue and a memory cue separately.

## Beat FX catalogue and picker

Maintain [docs/BEAT_FX.md](docs/BEAT_FX.md) alongside factory XML and native controls.
The Standard section uses versioned `[RB7] ` IDs with documented native
approximations. Preserve legacy/custom files, Pad FX IDs and the Saved section.
Never silently claim reverse, freeze, slip or transport behavior for delay chains.
Generate factory XML with `scripts/build/generate-beatfx.py`; keep tests in
`tests/effects/` and native audio/control regressions in `src/test/`.

The picker uses two columns and seven rows per page, with fixed 44px effect
buttons, 10px labels, 4px gaps and 4px outside padding. Utility buttons are
30px high and the page counter uses 9px text;
page changes and Close must never load an effect. Match selected state to the
live chain. Refresh the [picker gallery](docs/UI_SCREENSHOTS.md#beat-fx-picker)
and Play image for visible changes, together with changelog links.

Beat grid labels are periods in beats. Bind `parameterN_beat_period`; native
code converts Tremolo's cycles/beat and mirrors clamped values. Keep existing
raw controls and persisted parameter values unchanged. Verify both Echo and
Tremolo when changing time controls, including Echo's two-beat maximum.

## Docker disk hygiene

Follow [Docker maintenance](docs/DOCKER_MAINTENANCE.md) before large builds and
when recovering from disk exhaustion. Inspect host and Docker disk usage before
and after heavy builds; aim for at least 10 GiB free before starting and remove
obsolete task-owned outputs when space falls below that budget. When
the user requests full cleanup, run `docker system prune --all --volumes --force`
and report reclaimed space; do not substitute dangling-only cleanup. This
explicit global request permits pruning stopped instances across worktrees.
Keep running containers and persisted named-volume data outside that cleanup.
Recreate needed test instances through their owned-worktree launchers and read
the new ports. Do not prune globally on every build or restart a healthy engine.

The fullscreen `BeatFxPicker` is an explicit native exception to the kiosk's
dialog suppression. Do not remove that exception or broadly enable stock
modal dialogs. Use `HighContrast::mapStyleSheet` for its native styling and verify
both Night and Day after styling changes.

Verified picker coordinates at 1024×600: Standard `(564,40)`, Saved `(692,40)`,
Clear FX `(820,40)`, Close `(948,40)`; effect column centers `x=262,762`,
row centers `y=131,192,254,316,378,439,501`; Previous `(106,560)`, Next `(918,560)`.
Order is row-major; page 1 has entries 1–14, page 2 has 15–25. Selection uses
persisted preset IDs; page/section buttons never write `chain_selector`.

## Controller pad drawer

The Overview cue drawer follows the DDJ-400 mode selected on either deck.
Hot Cue restores the existing Hot Cues/Memory controls; Pad FX, Beat Jump and
Beat Loop replace the pad grid with a **read-only controller legend**. Use the
physical pads to perform the displayed actions. X still closes the drawer;
selecting a controller mode opens that deck again. Mode button release does
not close it. Other modes close only their own deck's visible drawer.

Runtime controls (not saved): `[PadFX],dN_mode` = 0 Hot Cue, 1 Pad FX,
2 Beat Jump, 3 Beat Loop, 4 Memory; `dN_shift` = 0/1; `dN_jump_bank` = 0/1/2 for
1/16, 1, 16 multipliers. The existing mapping shares jump sizes across decks.

The legend follows saved Normal/Shift assignments, timing overrides, strength
and toggle settings. Beat Loop shows four held rolls and four toggle loops;
holding Shift shows its mapped shifted Pad FX assignments. Beat Jump's Shift
bank shows size ÷16 / ×16 on pads 7/8. Modes are independent per deck.


## Touch drawer navigation and padding (1024×600)

- Open Deck 1/2 with the existing CUE chips `(60,462)` / `(572,462)` on Play.
- Drawer header: Previous `(116,476)`, mode label `(530,476)`, Next `(944,476)`,
  Close `(994,476)`. Header targets are 44px tall, arrows 48px wide.
- Previous emits `[PadFX],dN_previous`; Next emits the existing `dN_cycle`.
  Both emit press/release, with only press advancing. The label only reflects
  `dN_mode`; it does not change mode when tapped.
- Forward order: Hot Cues=0 → Memory=4 → Beat Jump=2 → Pad FX=1 → Beat Loop=3.
  Previous reverses and wraps. Each deck retains its independent mode.
- The 150px drawer keeps 8px horizontal and approximately 4px vertical outside
  clearance, 4px row gaps and at least 44px pads. Pad centers are approximately
  x=`132,385,638,891`, y=`525,574`; inspect current geometry before pad actions.
- Verify both directions, controller-to-touch mode handoff, Close, all five pages,
  and Day/Night. See [drawer gallery](docs/UI_SCREENSHOTS.md#controller-pad-drawer).

Waveform regressions: type and palette changes update Browse, Play and deck
summaries while paused. Do not restore filesystem access or `getTrack()` calls
in preview painting; cached summaries load on the background pool. Preserve
completion and waveform identity in the bounded pixmap cache.

## noVNC Delivery Gate

- Before delivering a test GUI URL, follow [the noVNC delivery gate](docs/GUI_TESTING.md#novnc-delivery-gate): pass fast and E2E JavaScript checks, then verify the connected desktop in a real browser. HTTP success alone does not establish a working client.
- Keep cached-image repair shared by automated and manual launch paths. Test fresh, malformed and repeatedly repaired sources; invalidate the full module graph when changing cached assets.

### Panel-contained Beat FX picker
The FX selector `(934,122)` opens a child picker bounded by
`x=852..1015, y=100..599`. Standard/Saved centers are `(894,119)` /
`(974,119)`, Clear FX/Close `(894,153)` / `(974,153)`. Effect columns are
`x=894,974`, row centers `y=194,242,290,338,386,434,482`; each target is
44px tall with 10px labels. Utility controls are 30px tall; the page counter
uses 9px text at `(934,557)`. Prev/Next: `(894,581)` / `(974,581)`. Standard pages contain
14/11 entries in row-major order. Saved identifiers and controller order
are unchanged. Page changes and cancellation preserve selection; leaving
FX dismisses the picker. See [the gallery](docs/UI_SCREENSHOTS.md#beat-fx-picker).

Compact FX actions: eraser = Clear FX, × = Close, left/right chevrons =
Prev/Next. Tooltips and accessible names retain the action labels. Standard
and Saved remain labeled tabs; the 9px page counter reads `1 / 2`.

## Branch-local Docker environment and old worktrees

- Launch and open branch previews using the [environment-variable recipe](docs/GUI_TESTING.md#environment-variables-for-launching-and-opening-a-branch-preview).
  Export task-specific `BITEDJ_TEST_*` values in that task's shell, source the
  settings helper, verify ownership, and discover actual ports before opening.
  `.env` files are not loaded automatically. Clear inherited overrides when
  changing tasks; never hard-code another instance's container name or URL.
- Finish every authorized integration with a cleanup audit, including the actual
  sibling folders on disk, Git worktrees, Docker labels and bind mounts, nested
  repositories, and ignored artifacts. Do not stop at `git branch --merged`:
  squash merges require checking the integrated source changes.
- Remove the completed task's inactive checkout once integration is verified.
  For squash merges, record the original tip and integration commit, verify
  exact tree identity or review the complete task diff and integration changes,
  and retain the original tip under a recovery ref when ancestry is absent.
  Use non-forced `git worktree remove`; follow the canonical policy for ref deletion.
- Clean known task-generated disposable caches and temporary build/test outputs
  when retiring that task. Preserve unique settings, user media, uncommitted work,
  deliverables and uncertain files. If preservation is needed, consolidate them
  under one ignored recovery directory with a manifest and report its size;
  moving artifacts does not reclaim disk space. Do not accumulate new sibling
  backup folders or archive reproducible caches indefinitely.
- Keep active tasks, integration checkouts and requested previews. Retire only
  the completed task's own preview when it is no longer needed for review; never
  stop another task's container or remove a bind-mounted folder. Report remaining
  folders with specific reasons, sizes and the next cleanup action.

## Service Deck Jog Smoothing

Service Preferences → Decks → Deck options adds Jog-wheel smoothing after
Clone deck (eighth row), stored as `[Controls] JogWheelFilterLength` (6, 1–64).
At 1024×600 with the native service window maximized, the spin box is `(600,302)`;
Apply is `(974,577)`. Full option order, verified coordinates and persistence
checks are in [GUI testing](docs/GUI_TESTING.md#service-deck-preferences-jog-smoothing).
