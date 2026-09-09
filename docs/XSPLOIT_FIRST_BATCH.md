# Approved first batch

Branch: `codex/xsploit-readme-feature-map`, based on local `v0.0.6`.
The user designated **`codex/v0.0.7`** as the eventual merge target.
Source: xsploit/bitedj at `4c1dfec590f98851159fe7a64e3348e8aad306a5`.
Approved by the user on 2026-09-08. No release bump, merge, deployment or push.

- [x] 1 / DOC-01: README identity, upstream credits, preserved background and NOTICE.
- [x] 2 / FX-01: Programmatic effect enable/disable reaches the audio engine.
- [x] 3 / LIB-01: Persist column order and text size; retain selection after sorting.
- [x] 10 / RB-01: Page traversal safety and resilient DAT/EXT analysis import.

Preserve the 1024×600 layout, FX/KEY overview tabs, native Beats grid,
custom Wayland track dragging, DDJ-400 mapping and existing USB override stores.
Record implementation details, provenance and actual test results below.

## Documentation

Reorganized the README and added upstream links and NOTICE. Preserved its previous
contents verbatim under a provenance header. Local Markdown targets and
`git diff --check` passed. No application behavior changed in this step.

## FX state publication (2 / FX-01)

Adapted xsploit commit `4c1dfec590`: a programmatic enabled-state change now
publishes to the engine; repeated identical states are no-ops. The new
`EffectSlotTest` processes an 8 kHz signal through our standard Filter rack,
checks attenuation when enabled, and verifies bypass restoration on release.
It also covers repeated values and external control writers. No Pad FX lanes imported.

## Library usability (3 / LIB-01)

Adapted the fork’s header-order approach and text-size persistence (`0144fc92b5`).
Managed order/sort state uses each model’s settings namespace in `mixxx.cfg`,
including proxy models; widths/visibility are reapplied from existing controls.
This avoids adding synchronous SQL writes to the drag handler. Font and row size
use the existing settings lifecycle, including persisted Detail-mode row height.

Sorting uses model row identity for SQL-backed external tables instead of
resolving their rows to local-library track IDs (which can also import analysis).
Proxy models forward that identity to their source. Existing playlist-position
selection, unsortable-column guards, 96-pixel preview minimum and custom Wayland
track dragging remain in place. The native fixture covers external IDs, proxy
forwarding, config reload, and order restoration without losing managed widths.

## Rekordbox reader (10 / RB-01)

Adapted the pinned PiFlex page guard and independent analysis-file wrapper.
Page indices retain 32 bits; zero/oversized page sizes, out-of-file references,
and repeated pages are rejected before traversal continues. DAT-only files run
both beat and cue passes. EXT cues remain preferred; a corrupt EXT preserves
existing cues instead of silently using older DAT cues. A missing DAT still
permits a valid EXT cue pass. Failures warn and leave audio loading available.
Per-USB cue/rating overrides still apply after import.

Fixtures exercise real generated ANLZ parsing, DAT grids/cues, EXT preference,
truncation, missing files, and page bounds/cycles including indices above 65535.
This batch does not add `.2EX` waveform/phrase import, analysis-policy controls,
OneLibrary support, or changes to shared-analysis-path database constraints.

## Independent tests and agent workflow

The user requested reproducible branch isolation during implementation. The root
agent guide (normalized to `AGENTS.md`) preserves the existing UI/debugging rules
and points to `docs/AGENTS.md`. Both require a feature branch and worktree for each
new task, an explicit semver merge target, Conventional Commits, and isolated tests.

`scripts/test/run-gui-test.sh` uses ownership labels, distinct settings/results directories,
and automatically assigned localhost ports. Fixed ports are optional. The FX and
preview scripts use the same instance selection and port discovery. No developer
USB path is mounted automatically; optional `BITEDJ_TEST_USB_DIR` is read-only.
The test page identifies its branch and exposes revision/binary SHA-256 at
`/branch.json`. See [GUI_TESTING.md](GUI_TESTING.md) for commands.

Verified: rejection of the legacy container without modifying it; an additional
automatic-port instance alongside both live instances; cleanup of only that probe;
and fixed-port startup at 6081/8001/5901. Shell syntax checks pass. The old
`test-gui-automated.sh` mentioned in earlier guides is absent, so endpoint,
screenshot, and focused native checks were used instead of claiming it ran.

## Validation and remaining qualification

- ARM64 Docker application build completed using the repository build script.
- All **22 focused native tests pass**, including proxy namespace/row identity,
  header config reload, FX audio behavior and Rekordbox parsing.
- All **13 related native tests pass**: controller column IDs, played-track state,
  and touch scrolling/taps. Total: **35 passing native tests**.
- VNC and audio HTTP endpoints returned 200; the isolated audio stream produced
  40,960 bytes in a bounded two-second capture (not proof of audible output).
- Live window geometry is 1024×600. Header dragging moved Title before Artist;
  the reordered layout survived a complete owned-instance restart.
- The running `/proc/<pid>/exe` hash matches the installed ARM64 binary:
  `c207df0f9b3b89100e80a2ffd0af3f8307c4079563f6ea863f0defda99ee390e`.
- The other live container’s ID/start time remained unchanged throughout the
  parallel-instance checks. No hardware deployment or shared-instance replacement.
- Physical Wayland touch/DDJ-400 checks, real Rekordbox USB disconnect/reconnect,
  and sustained Pi audio/xrun qualification remain hardware acceptance work.

The selected changes will be merged later into **`codex/v0.0.7`**. This worktree
retains inherited version 0.0.6; no hardware deployment, release bump, merge or push.

## Reproduce the native checks

After the ARM64 application build, configure and run the test target in the same
builder image and this worktree (no host Qt build):

```bash
docker run --rm --platform linux/arm64 \
  -v "$PWD:/src" -v bitedj-ccache:/root/.cache/ccache \
  -w /src/build-linux -e QT_QPA_PLATFORM=offscreen \
  bitedj-builder-linux-arm64 bash -ec '
    cmake -S /src -B . -DBUILD_TESTING=ON -DBUILD_BENCH=OFF
    cmake --build . --parallel 8 --target mixxx-test
    ./mixxx-test --gtest_filter="EffectSlotTest.*:LibraryColumnControlTest.*:RekordboxAnlzTest.*:RekordboxPageChainTest.*:ControllerLibraryColumnIDRegressionTest.*:TouchScrollFilterTest.*:PlayedTracksTest.*"
  '
```

Live instance: `bitedj-xsploit-gui`, noVNC `http://localhost:6081/vnc.html`,
audio `http://localhost:8001/stream.mp3`, VNC `localhost:5901`.
Launch with the explicit variables in [GUI_TESTING.md](GUI_TESTING.md).
Local screenshots/logs are under `test-results/bitedj-xsploit-gui/` (ignored by Git).
A passing desktop test is not a replacement for physical Pi/USB/controller acceptance.
