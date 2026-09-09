# Test strategy and suite

Worktree: `codex/test-suite`. Intended merge target: `codex/v0.0.7`.
Start future tasks from the latest agreed semver integration branch, not an older
checkout. Keep build outputs and GUI instances local to that task's worktree.

## Run the appropriate layer

Run commands from the repository root; the runner also resolves correctly when
invoked by absolute path from another directory. Python 3.9+ and Node.js 22+
are needed for the fast suite. Native tests need a configured, compiled
`mixxx-test` (`BUILD_TESTING=ON`); the runner never rebuilds implicitly.

| Command | Coverage | When to run |
| --- | --- | --- |
| `python3 scripts/test/run-tests.py fast` | All DDJ-400 MIDI bindings and Pad FX behavior with a fake clock; skin XML, template references, tab contracts | Every edit; first CI gate |
| `python3 scripts/test/run-tests.py bitedj --build-dir build-linux` | Focused BiteDJ C++ behavior: settings, DSP routing/tails, UI widgets, library, import and controller regressions | Local feature changes |
| `python3 scripts/test/run-tests.py native --build-dir build-linux` | Full C++ suite except tests needing removable mounts | Every PR and push to main/develop/semver branches |
| `bash scripts/test/run-removable-tests.sh --build-dir build-linux` | Removable storage, cue/metadata/history persistence, sampler unplug/replug | Storage changes; every native CI run |
| `python3 scripts/test/run-tests.py e2e` | Real desktop navigation and reselect, audible playback/stop, noVNC resources and decodable audio transport | GUI/audio changes; semver pushes and optional CI dispatch |
| `python3 scripts/test/run-tests.py all --build-dir build-linux` | Fast, full native, removable and desktop layers | Complete local gate on a prepared Linux host |

`native` and `bitedj` exclude the `removable` label deliberately. Run the
removable layer as well for complete coverage. That layer creates a private
Linux mount namespace and temporary filesystems in separate store and sampler runs, so the single-drive
case never sees the store fixture's extra USB drive. It fails if mount support is
unavailable or any selected test skips. Non-root Linux runs require enabled
user namespaces. CI uses `sudo` for the private namespace. In Docker, the
removable fixture needs mount privileges, for example an explicitly isolated
builder container with `--cap-add SYS_ADMIN --security-opt seccomp=unconfined`.
Do not mount real USB drives into that test container.

C++ tests run serially by default because mount fixtures use fixed paths and
some existing tests share process/filesystem assumptions. Removable cases also
have a CTest resource lock. `--jobs N` permits measured experimentation with
native parallelism; do not increase the CI default without auditing fixtures.
Native JUnit reports go to `<build-dir>/<suite>-results.xml`.

## Desktop E2E setup

Build this worktree using `scripts/build/docker-build.sh`, then build the GUI
runtime once. Check `df -h` and `docker system df` before large builds; clean
only task-owned outputs. Do not restart a healthy engine or run global prune
commands without explicit cleanup authorization.


```sh
docker build --build-arg BITEDJ_BUILDER_IMAGE=bitedj-builder:latest -t bitedj-gui-test:latest -f docker/gui-test.Dockerfile .
python3 scripts/test/run-tests.py e2e
```

Pass the matching builder tag explicitly as `BITEDJ_BUILDER_IMAGE`. The example
uses `bitedj-builder:latest`, which CI builds from Ubuntu 24.04; for local builds,
substitute the platform-specific tag produced by the build script. Omitting the
argument selects the Dockerfile's ARM64 default, which may be absent or mismatch
the CI binary. Do not pull a local builder tag from a registry.
E2E does not build images or application binaries implicitly. Rebuild the
application after C++ changes; current skin/effect resources are mounted from
this worktree. Do not use an old release binary to validate new C++ behavior.

The suite calls the shared GUI launcher in automated mode. Each run owns a
unique, labeled container with fresh settings, a synthetic 440 Hz WAV, and
no published host ports. It never replaces the interactive test instance or
changes `test-config/active-instance`. Containers are removed at completion;
PNG evidence and service logs remain under `test-results/bitedj-e2e-*/`.
Startup and polling have deadlines, and command failures include stderr.

Assertions check selected tab pixels at the documented 1024×600 geometry,
including tapping an already-selected tab. Audio checks decode the live MP3
stream to PCM, verify silence before playback, audible output after pressing
Play, and silence after stopping. This verifies the desktop-to-audio path;
it does not establish physical device latency or speaker quality.

The old `test-gui-fx.sh` and `test-gui-preview.sh` paths remain compatibility
wrappers for this smoke suite. They no longer claim that screenshots alone
prove preview analysis, drag/drop, or effect modulation.

## CI cost and coverage

The fast job runs before the expensive native compilation. Ccache and the
existing full native gate are retained. The native and removable selections
are disjoint, so mount-dependent cases run with their prerequisites rather
than appearing as skipped successes. Both publish JUnit results.

Pushes to semver branches named `codex/vMAJOR.MINOR.PATCH`,
`codex-vMAJOR.MINOR.PATCH`, or `vMAJOR.MINOR.PATCH` automatically include desktop
E2E (including prerelease suffixes). Pull requests still run fast/native coverage.
Use the **Tests → Run workflow → e2e** input to include desktop E2E on other
branches. The E2E steps reuse
the already-built native library to build/install `mixxx`, and builds a GUI
runtime based on Ubuntu 24.04 to match the CI binary. This avoids compiling the
application twice and avoids putting Docker startup on the fast feedback path.
Once compilation succeeds, E2E still runs if a native assertion fails, while
the job retains that native failure.

## Post-merge CI check and repair

Every merge and push to the active SemVer branch includes a CI follow-through.
Use the active branch in [BRANCH_VERSIONING.md](BRANCH_VERSIONING.md), currently
`codex/v0.0.7`, and the user's `origin` repository. Record the exact published
commit, workflow run URLs, outcomes and repairs in the task checklist.

```sh
git fetch origin codex/v0.0.7
published_sha=$(git rev-parse origin/codex/v0.0.7)
gh run list --repo pablo-feijo/custom-bitedj --branch codex/v0.0.7 \
  --commit "$published_sha" --json databaseId,workflowName,headSha,status,conclusion,url
gh run view RUN_ID --repo pablo-feijo/custom-bitedj --json headSha,status,conclusion,jobs
gh run view RUN_ID --repo pablo-feijo/custom-bitedj --log-failed
```

1. Confirm runs belong to that SHA. PR checks and earlier successful commits do
   not validate the merged result. If no run appears, inspect workflow triggers
   and dispatch the Tests workflow with `e2e=true` if necessary; verify its SHA.
2. Follow all expected workflows to completion with bounded status checks. For
   Tests, verify fast tests, native build/tests, removable integration, desktop
   build, matching runtime build and desktop E2E all succeeded. Missing, pending,
   cancelled or unexpectedly skipped checks are unresolved, not passes.
3. Read failed step logs and uploaded test reports, find the cause and fix it in
   the task's isolated feature worktree. Run the relevant local checks. Do not
   disable tests, weaken assertions or repeatedly rerun deterministic failures
   to obtain green CI. Retry a transient infrastructure failure only after
   identifying and recording the reason.
4. When integration is authorized, squash the repair into the agreed SemVer
   branch and push, then repeat verification for the new published SHA. A newer
   push can cancel the previous run; follow the replacement and record both SHAs.
   Preserve other tasks' work and existing merge permissions.
5. Finish only after all expected checks pass, or explicitly report the blocker,
   failed/pending run links and remaining action. Keep unpublished repairs and
   externally blocked checks open; do not claim CI success from local tests.

## Review findings and remaining coverage

- Preserve the native suite; widget behavior, storage, controller state and DSP
  sample comparisons are cheaper and more diagnostic there than through clicks.
- Keep controller logic tests in the fast layer: the existing fake-clock Pad FX
  suite covers holds, retrigger, tails, shifted release, corruption and shutdown.
- The former FX script compared different MP3 chunks recorded at different
  times. That could pass without an effect. Deterministic `EffectSlotTest`,
  `PadFxRoutingTest` and `PadEchoTest` provide the native effect assertions.
- The former preview script clicked coordinates and captured images without
  checking results. Automated preview-analysis and drag/drop journeys remain
  coverage gaps; use the [Rekordbox fixture procedure](../tests/rekordbox/README.md)
  for manual evidence until they have semantic or reviewed visual assertions.
- Preserve the deeper virtual-MIDI/audio checks in
  [Pad FX testing](PAD_FX_TESTING.md). They remain an explicit feature/release
  gate because they have additional compiler, controller and owned-instance
  prerequisites; the smoke suite does not replace their DSP measurements.
- Add regressions at the lowest layer that catches the failure. Grow desktop
  E2E only for behavior crossing components; screenshots are evidence, not a
  success condition by themselves. Avoid retries of whole failed tests.

## Refactor validation checklist

- [x] Base isolated feature work on `codex/v0.0.7`.
- [x] Run fast controller/resource checks on that baseline.
- [x] Validate Python and shell syntax and CTest label registration.
- [x] Run native/removable tests against the matching application build.
- [x] Run the complete desktop E2E suite on this worktree's binary.

Runtime validation reuses an independent copy of the existing `0.0.7` build
whose application source matches the integration base; CTest is reconfigured
for this branch's labels. No older-release results count toward this checklist.

Validation on 2026-09-09:

- Fast: controller checks and four resource contracts pass (~0.2 seconds total).
- Focused native: 89 pass (~13 seconds).
- Removable: 42 store + 12 sampler cases pass, with no skips (~16 seconds).
- Desktop: all three E2E cases pass (~17 seconds). A startup focus race found
  during repetition was addressed by activating the window before establishing
  the initial page and allowing the documented menu-settling interval.
- Full native: 974 pass, 1 fails, 13 were already disabled (~168 seconds).
  The unchanged `SoundSourceProxyTest.firstSoundTest` reports sample 3326 versus
  reference 3376 for `cover-test-vbr.mp3` using ARM64 libmad `FPM_DEFAULT`.
  It also fails when invoked directly without the new runner. This is an open
  decoder/reference investigation; no exclusion or relaxed assertion was added.
- GitHub Actions configuration parses; the remote workflow has not been run.

## Preview and waveform regressions

`WaveformRenderingTest` checks empty/partial summaries, full-track coordinates,
transient-preserving downsampling, palette round trips, 3 Band stacking and deck
loading with stale duration controls. `PreviewDelegateTest` exercises actual
painting, pixmap reuse, completion/replacement invalidation, row reordering,
metadata-only tracks, clearing and a 300-track bounded-cache traversal. Its model
counts metadata-loading calls so accidental track imports from paint fail tests.
`GlobalTrackCacheTest.PreviewLookupSkipsUnpublishedTrackWithoutWaiting` exercises
an unfinished metadata import: waiting for that import would deadlock the test.

Run the focused native selection with:

```sh
./scripts/build/docker-build.sh --platform linux/arm64 --test-filter 'WaveformRenderingTest\.|PreviewDelegateTest\.|GlobalTrackCacheTest.Preview|PadFxSettingsTest.Touch|PadFxEditorTest.Controller'
```

The desktop E2E suite also compares actual paused Play/deck pixels after RGB,
Filtered, 3 Band and palette changes, including a round trip without reloading
or moving the track. GPU and USB throughput on a physical Pi remain separate
from the container checks. Fast tests include controller drawer mode bindings;
native touch tests cover both directions and release handling.

For an owned interactive cold-cache/replacement check, generate an additional
fixture with `python3 tests/e2e/make_preview_track.py test-music/Preview_Replacement_16s.wav`.
Enable the home analysis cache for the read-only synthetic `/music` mount
(`AnalysisCacheOnTrackFs=0`, `AnalysisCacheInHome=1` in test settings), load and
analyze the 16-second fixture, then restart with the normal 60-second deck
fixtures. Browse must show the unloaded short track's summary from disk; its
four sections must stay in place after type/palette changes and when loaded
into a deck. The test settings belong only to that owned fixture instance.


After a fresh owned GUI start, `python3 tests/e2e/check_touch_drawer.py` checks
all five distinct header labels, forward wrap and the entire reverse cycle on
both decks using captured pixels. It needs no controller and leaves the drawer
closed. Captures are saved under that instance's ignored test-results directory.
