# AI Agent Instructions

<!-- Modified for Custom Bite DJ on 2026-09-09: clarify fork identity and attribution. -->

Hello! If you are an AI assistant or autonomous agent (like Antigravity, Claude, or GitHub Copilot) working on this codebase, **read this document before making changes.**

Custom Bite DJ is an independent fork of [Team Deckshark’s BiteDJ](https://github.com/TeamDeckshark/bitedj), itself based on [Mixxx](https://github.com/mixxxdj/mixxx), specifically engineered to run as a headless **Raspberry Pi OS Appliance** using the **Sway/Wayland compositor** and a multi-touch screen.

## Branch and Test Isolation — Required for Every New Task

- Integrate each task into a semver branch with a squash merge: one Conventional Commit for the complete task, without bringing its intermediate commits or a merge commit into the semver history.
- Rewrite previously published semver commits only when explicitly requested. Preserve unrelated history and a local backup ref, and push rewritten history with an explicit expected-value force-with-lease.

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
- Fetch the latest active unreleased base, currently `origin/codex/v0.0.7`,
  before starting new work. Track release transitions in [BRANCH_VERSIONING.md](BRANCH_VERSIONING.md).
- Work-branch binaries must embed a branch-specific SemVer prerelease before
  building artifacts. Follow the synchronization and verification protocol below.
- Every worktree must have its own `build-linux/`, `dist-linux/`, test settings,
  results, and GUI container. Never share writable build/install/config directories
  between branches. A shared compiler cache and immutable Docker image are fine.
- Use `scripts/test/run-gui-test.sh` and `scripts/test/gui-test-settings.sh`: they derive a worktree-specific
  container name, assign free host ports by default, and refuse to replace a
  container labeled for another worktree/branch. Explicit names/ports may be set
  with `BITEDJ_TEST_INSTANCE`, `BITEDJ_TEST_WEB_PORT`, `BITEDJ_TEST_AUDIO_PORT`,
  and `BITEDJ_TEST_VNC_PORT`.
- Read the printed endpoints or `docker port`; **never assume port 6080 or the
  legacy `bitedj-gui-test-instance` belongs to this task**. Source
  `scripts/test/gui-test-settings.sh` and run `verify_test_instance_owner` before manual
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

## Post-Merge CI Check and Repair

After every merge and push to the active SemVer branch, follow
[the CI check and repair procedure](TESTING.md#post-merge-ci-check-and-repair).
Record the published SHA and run links, wait for all expected jobs (including
SemVer desktop E2E), investigate failed logs, and fix errors within the task.
Validate each repair and recheck the resulting published SemVer commit until
CI passes. Do not treat a merge, successful PR checks, a cancelled run, or a
partial green job as completion. Preserve branch isolation and existing merge
authorization; record any external blocker or unpublished repair explicitly.

## Branch Cleanup After Integration

- Include branch hygiene in task completion. After an authorized merge and push,
  remove the task's fully merged feature branches locally and on the user's
  remote when the checks below pass. A cleanup request authorizes this routine
  cleanup; do not repeatedly ask for confirmation. Cleanup does not authorize
  additional merges, pushes of unrelated work, or discarding unmerged commits.
- Preserve `main`, the remote default branch, all semver/release branches
  (including `codex/v0.0.7` and `v0.0.2-effects`), and all tags. Preserve unmerged
  branches regardless of their age or name. Do not delete branches on third-party
  upstream remotes; identify the user's remote from its URL, not its name alone.
- Fetch and prune the user's remote before auditing. Inspect local and remote
  refs, working-tree status, `git worktree list`, and live test-instance ownership.
  Use full ref names to avoid branch/tag ambiguity and exclude symbolic refs
  such as `origin/HEAD` from deletion candidates.
- For branch deletion, prove each candidate tip is an ancestor of a retained
  integration/release ref with `git merge-base --is-ancestor`. For remote deletion,
  also prove the tip is reachable from a retained, published remote ref. Matching
  filenames or commit messages are not proof. Squash-merged checkout removal is
  separate: verify exact tree identity, or review all task changes against the
  integration result and account for every difference. Record that evidence and
  preserve the original commit under its existing branch or a recovery ref;
  lack of ancestry alone must not leave a redundant checkout on disk.
- Prefer `git branch -d`. If Git refuses because it checks a different upstream
  or current branch, use `-D` only after independently proving ancestry to the
  intended retained ref. Never force-delete unique or uncertain history.
- Do not detach, switch, remove or overwrite another active task's checkout.
  Keep local branches needed by running GUI instances: changing their branch
  breaks the ownership check. Record the retained branch and reason, then finish
  cleanup when that task's instance is retired. A branch-only cleanup does not
  authorize stopping a GUI requested for review.
- Remove inactive, completed task worktrees after the integration audit; do not
  merely detach them and leave the folders. Check tracked changes, untracked and
  ignored outputs, and nested submodule worktrees first. Delete only identified,
  disposable task-generated outputs; preserve unique or uncertain data with a
  recovery manifest. Do not use forced worktree removal or blanket `git clean`.
- For nested repositories, inspect their own worktree lists before removing a
  parent directory. Retire verified redundant nested checkouts first, preserve
  refs and pinned commits (a verified `git bundle --all HEAD` is a recovery option),
  and preserve repository metadata if Git cannot remove it normally. Never delete
  a Git common directory still used by another checkout.
- After local-only integration, the completed checkout may be retired while its
  branch/recovery ref remains pending publication. Do not infer authorization to
  push, delete remote refs, prune Docker globally or discard historical release
  archives from a local squash-merge request.
- Audit submodule repositories separately. Preserve any branch needed to keep a
  parent repository's pinned gitlink reachable remotely. Publish a retained ref
  containing that commit before deleting its last remote branch; never assume a
  parent merge also merged the submodule's feature branch.
- Delete only the audited branch names, prune stale tracking refs, then verify
  the remote refs and local status. Report removed branches and any retained
  exceptions. Leave unrelated files, settings, builds and test results intact.

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

For every visible UI change, follow the root
[published screenshot policy](../AGENTS.md#published-ui-screenshots): recapture
all affected documentation images, update gallery captions and link the affected
gallery section from that change's changelog entry in the same commit. Preserve
released galleries. Curated documentation screenshots are committed assets;
raw test screenshots and logs remain ignored.
Every UI option addition, removal, rename, move or resize must update the root
[UI guide](../AGENTS.md) and [GUI testing guide](GUI_TESTING.md) in the same
commit. Record option order, measured 1024×600 coordinates and control/value
mappings; update affected automation and controller docs. Preserve keys, enum
values and saved page indices for layout-only changes. Verify labels, touch
clearance, padding and footer visibility in the owned VNC instance; include
Day/Night checks when styling changes. Prefer a canonical mapping link over
stale duplicate coordinates.

## Attribution and Licensing

- Identify this project as **Custom Bite DJ**, an independent fork of Team
  Deckshark's BiteDJ, based on Mixxx. Link upstream and distinguish inherited
  work, adaptations and local changes; do not imply endorsement or upstream support.
- Preserve copyright notices, author credits and license texts. Keep source
  identifiers, paths and historical documents accurate; never globally replace
  “BiteDJ” or “Mixxx” inside notices, code keys or third-party material.
- Follow [LICENSING.md](LICENSING.md) when changing attribution or preparing
  distribution. The program and skin have distinct license notices. Document
  upstream paths/revisions and dated modifications when adapting material.
- Recheck component terms before importing code or artwork. Credits alone are
  not permission, and a documentation review is not a complete release audit.

## Repository Organization

- Follow [the canonical repository layout](REPOSITORY_LAYOUT.md) for every new file.
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

## Docker maintenance

Use the canonical [Docker cleanup and disk recovery guide](DOCKER_MAINTENANCE.md).
Check disk space before large builds. For an explicitly requested full prune,
use `docker system prune --all --volumes --force`, report reclaimed space and
recreate removed stopped GUI environments from their own worktrees. This is the
explicitly authorized exception to routine container-isolation rules. Preserve
running containers and named-volume data; do not globally prune on every build.

## 2. Infrastructure & Build Workflows

If the user asks you to compile or test the application, use the scripts under `scripts/`, as listed in [the repository layout](REPOSITORY_LAYOUT.md):

- **Compiling for the Pi**: Run `./scripts/build/docker-build.sh --platform linux/arm64`. This uses a custom Docker container to cross-compile the binary into `dist-linux/`. Do not try to compile natively on a Mac or Windows machine using standard `CMake` unless you are explicitly building a local debug version.
- **Local GUI & Audio Testing**: Before deploying changes or building an OS image, verify the 1024×600 GUI and affected audio paths in the owned instance using [GUI_TESTING.md](GUI_TESTING.md), [PAD_FX_TESTING.md](PAD_FX_TESTING.md), and the [Rekordbox fixture procedure](../tests/rekordbox/README.md) as applicable. Start interactive testing with `./scripts/test/run-gui-test.sh` and use its printed VNC/audio endpoints; never assume ports belong to this task.
- **Hot-Deploying**: Use `./scripts/deploy/deploy-ssh.sh` to push a newly compiled ARM64 binary to a live Raspberry Pi over the network.
- **Flashing the OS**: The complete Raspberry Pi OS is generated using `./scripts/build/generate-pi-image.sh`, which leverages the `mixxx-pi-gen` submodule.

## 3. Important Context
Before attempting large refactors or upstream cherry-picking from `mixxxdj/mixxx`, review the following documents:
- `docs/DIFFS_FROM_BASE.md`: Master ledger of all C++ engine and OS changes vs upstream.
- `docs/INFRASTRUCTURE.md`: Explains Polkit permissions, Kernel realtime scheduling (`preempt=full`), and Docker dependencies.
- `docs/DDJ400_MAPPING.md`: Explains the custom Pioneer DDJ-400 Pad FX logic and Hardware UI interception.

## 4. Versioning Protocol

Active unreleased integration base: `origin/codex/v0.0.7` (target `0.0.7`).
Follow [Branch bases and binary versioning](BRANCH_VERSIONING.md) for the canonical
active-release record, branch creation procedure, build version format and release
transition history. Update that record and root `AGENTS.md` when the target changes.
Every work-branch binary must compile a branch-specific SemVer prerelease, e.g.
`0.0.7-codex-controller-pad-drawer.1`; filenames alone are insufficient.
Synchronize all produced artifacts and verify the actual rebuilt binary version.

For the 0.0.7 working release, pi-gen uses `codex/v007-custom-defaults`;
`codex/v0.0.7` is only the later merge target in both repositories. Commit on
the feature branch, publish it, then commit the parent `mixxx-pi-gen` gitlink.
Do not advance either semver branch without an explicit merge request.
Keep `.gitmodules` URL/branch valid so a recursive clone resolves the pinned
commit. The flasher derives IMG_NAME from the pinned config; use
`BITEDJ_IMAGE_DATE=YYYY-MM-DD` to select a build from a different day.
Do not copy UI resources into pi-gen: it consumes the matching parent ARM64
`dist-linux` build. Its sample mixxx.cfg is not installed on first boot.

## 5. Prevent Configuration Drift (Infrastructure as Code)
When resolving bugs on live hardware or applying hot-patches over SSH (e.g., editing `~/.config/sway/config` or modifying `gsettings` on the Pi), you **must immediately backport those changes to the local repository.** 
- Never leave a live Pi in a state that cannot be exactly reproduced by `./scripts/build/generate-pi-image.sh`.
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

## Reproducible Test Assets

Keep test generators, synthetic fixture definitions, reusable scripts and test
procedures in Git. Put generated exports, audio captures, screenshots, logs,
benchmark snapshots, caches and test reports in ignored `test-results/` (or
other ignored runtime directories). Do not commit test-run results. Record
instance ownership and regenerate assets instead of copying personal music.

## Branch-local Docker environment and old worktrees

- Launch and open branch previews using the [environment-variable recipe](GUI_TESTING.md#environment-variables-for-launching-and-opening-a-branch-preview).
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
