# Test strategy and suite

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
At initial startup and after the saved-playlist fixture restarts, Browse must
remain selected for two seconds before row/toolbar clicks. This covers the
controller startup watchdog's 1.5-second redirect to Devices; a single successful frame can precede that
redirect. Queue count, duplicate ordering, removal and audio assertions still
run against the real desktop and database.

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

## Reuse compiled outputs for asset changes

CI fingerprints tracked compiled inputs, build/workflow configuration, installed
package versions, runner architecture and workspace path. An exact binary-cache
hit skips both C++ build commands. Configure still regenerates CTest registrations;
fast, native, removable and the requested desktop tests all run again.

| Change | Compilation strategy |
| --- | --- |
| Skins, controller mappings, effect presets, keyboard mappings | Reuse exact compatible binaries; replace installed asset directories from this checkout |
| Root README/changelog/agent guide, `docs/`, native skill Markdown/`agents/openai.yaml`, or `.codex/config.toml` | Reuse exact compatible binaries; tests still run |
| C++, native tests, CMake/product version, workflow, dependencies, unknown inputs | New binary fingerprint; compile with Ninja, ccache and resource-bounded workers |
| Any file embedded through a Qt `.qrc` | Invalidate binaries even if located in an otherwise reusable asset directory |
| Cache absent, expired or evicted | Build normally and populate the cache; never restore an approximate binary match |

`ci-build-cache.py` preserves the original binary version, branch and commit in
`provenance.json`. These are reusable CI test binaries, not newly versioned release
artifacts. Do not relabel them or use this shortcut for a deliverable build.
Replacing whole asset directories removes deleted files as well as copying changes.
Tests exercise the current source assets; a cache hit never counts as a test pass.
Native skill scripts and other assets remain fingerprinted; only instruction
Markdown and the named UI/config metadata are excluded, unless embedded by Qt.

The Tests workflow uses Node 24 Action runtimes; its controller test interpreter
remains Node.js 22. The GCC problem matcher is registered from a local JSON file.

Compilation uses Ninja and up to four workers, limited by available CPUs and
3 GiB RAM per compiler. Native test execution remains serial because fixtures
share state. CI keeps the existing RelWithDebInfo optimization/debug flags;
changing those or enabling precompiled headers would require fresh compiler
cache entries and separate compatibility/performance validation.

The compiler cache is bounded to 4 GiB, uses compiler-content checks and is saved
immediately after compilation, including partial work after a build failure when
possible. Separate restore/save steps preserve it before later test/E2E failures.
The E2E Docker image also has an exact cache, keyed by its Dockerfiles, copied
helpers, installed host dependencies, architecture and a weekly refresh generation.
Cache storage/visibility follows [GitHub's branch-scoped cache rules](https://docs.github.com/en/actions/reference/workflows-and-actions/dependency-caching);
misses and eviction are expected. The first run must populate these new caches.

Validate changes to this strategy with the fast regression suite, workflow lint,
and two remote runs: a cold build followed by a compatible docs/asset-only run.
Confirm the second run reports an exact hit, skips compilation, restores original
provenance, refreshes assets and passes every test layer. A changed C++/CMake or
embedded-resource fingerprint must rebuild; the fast suite tests invalidation and
stale-asset removal. See [actions/cache](https://github.com/actions/cache) for the
exact-hit output and explicit restore/save actions used here.

The first validated cache-populating run was
[34347232331](https://github.com/pablo-feijo/custom-bitedj/actions/runs/34347232331)
for `8f428a8772`: four workers, 26m 36s native compilation, 1,035 native tests,
54 removable tests and five desktop E2E tests passed. All three caches were saved.
The old Node 20 Action warning is absent. Compilation had 939 cache misses;
this timing is a cold baseline, not a measured warm-cache result.

## Post-merge CI check and repair

Every merge and push to the active SemVer branch includes a CI follow-through.
Use the active branch in [BRANCH_VERSIONING.md](BRANCH_VERSIONING.md), currently
`codex/v0.0.8`, and the user's `origin` repository. Record the exact published
commit, workflow run URLs, outcomes and repairs in the task checklist.

```sh
git fetch origin codex/v0.0.8
published_sha=$(git rev-parse origin/codex/v0.0.8)
gh run list --repo pablo-feijo/custom-bitedj --branch codex/v0.0.8 \
  --commit "$published_sha" --json databaseId,workflowName,headSha,status,conclusion,url
gh run view RUN_ID --repo pablo-feijo/custom-bitedj --json headSha,status,conclusion,jobs
gh run view RUN_ID --repo pablo-feijo/custom-bitedj --log-failed
```

1. Confirm runs belong to that SHA. PR checks and earlier successful commits do
   not validate the merged result. If no run appears, inspect workflow triggers
   and dispatch the Tests workflow with `e2e=true` if necessary; verify its SHA.
2. Follow all expected workflows to completion with bounded status checks. For
   Tests, verify fast tests, native tests, removable integration and desktop E2E all succeeded.
   Compilation and runtime construction may be skipped only when their exact
   caches are successfully restored and verified; record that reuse explicitly. Missing, pending,
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

Priority and completion status for the gaps below are consolidated in the
[roadmap](DISPLAY_AND_DECK_LAYOUT_PLAN.md#consolidated-follow-up-backlog). This
section remains authoritative for test-layer detail.

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

The desktop `test_waveform_rgb_preview_matches_play_colors` compares the dominant
waveform color in the Play lane and bottom deck preview using constant bass,
midrange and treble tones. Both resolutions therefore represent the same frequency
content. It checks BiteDJ/Amber palette round trips and switching Filtered and
3 Band back to RGB, rejecting a preview stuck on green. Neutral overlays are excluded and a small
color tolerance allows for rasterization. This is a color-consistency check, not
an assertion that an entire music-track summary matches one zoomed-in passage.

`RekordboxImportTest` also covers loading a track whose native cached bands differ
from the export: the exported detail and summary must win for Rekordbox-loaded
tracks. Import runs on the analyzer worker, and missing/invalid exports leave
native cache and audio analysis available. A late batch analysis must not overwrite
an exported pair, and browser previews retain export colors while that batch runs.

`WaveformRenderingTest` checks empty/partial summaries, full-track coordinates,
transient-preserving downsampling, palette round trips, 3 Band stacking and deck
loading with stale duration controls. `PreviewDelegateTest` exercises actual
painting, pixmap reuse, completion/replacement invalidation, row reordering,
metadata-only tracks, clearing and a 300-track bounded-cache traversal.
It also covers a cached miss becoming partial/complete deck analysis, unloading
the track without losing its preview, and clearing after palette invalidation. Its model
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

## Recording reliability

`EngineRecordTest`, `EngineSideChainTest` and `RecordingManagerTest` cover saved
PCM samples and finalized WAV headers, file splitting, Linux `/dev/full` write
failure and recovery, unavailable destinations, 64-bit positions beyond 2 GiB,
filename collisions, low-space notifications, and interruption/error reporting.
The sidechain test blocks its consumer while submitting four seconds of audio;
it verifies that all samples drain after release. Overflow during a pending stop
or split must report failure instead of a successful save.

`EngineRecordTest.RecordingDoesNotForceWriteback` records past multiple former
1 MiB boundaries in a Linux child process that rejects `sync_file_range` using
seccomp. This catches forced writeback even on fast test filesystems where USB
request-queue stalls cannot be reproduced. Test setup failures fail the check.

`TagLibTest.MetadataImportNeverOpensForWriting` rejects writable `openat`
attempts while importing metadata and cover art from eight audio formats.
Imports use an explicitly read-only TagLib stream: closing a writable FAT
handle can flush pending writes from a recording on the same drive. Existing
TagLib export tests still verify intentional metadata writes.

Recording writes remain outside the audio callback. The sidechain has a 4 MiB
FIFO (about 11.9 seconds of stereo float samples at 44.1 kHz) and drains in
256 KiB chunks with background scheduling. This absorbs temporary stalls; it
cannot compensate for an arbitrarily slow or disconnected destination. Overflow
stops recording with an error and attempts to finalize the partial file.

For hardware validation, use a task-owned binary and coordinate exclusive Pi
input with other active tasks. Play WAV files on both decks from the USB drives,
start recording through the System drive row, enable the browser Preview column,
and browse continuously. Stop recording, wait for completion, and inspect the
actual file in the drive's `Recordings` directory: decode it, check duration and
sample continuity, and compare against a simultaneous output monitor where
available. Repeat start/stop and confirm earlier files remain intact. Review
playback-reader errors, sidechain overruns and callback timing alongside the
recorded audio. Keep generated recordings and measurements in ignored task
results; record the binary version, USB devices, source files and test duration.
A passing run establishes those tested conditions, not arbitrary hardware loads.


## Pi recording and load regression coverage

On 2026-09-11, candidate `0.0.8-codex-pi-ddj-recording.6` passed 37 focused native
checks, the fast suite, 13 image checks and all eight desktop E2E cases on retry.
The first E2E run had a saved-playlist navigation failure. Pi validation included
large WAV playback, DDJ Load actions from Rekordbox, History and Computer folders,
and repeated Browse/Play and settings transitions while recording to the same
USB drive. The user subsequently confirmed normal controller/audio operation.

The BFQ comparison captured 16:08.945 of continuous audio across 40 varied-track
batch loads. That diagnostic take required WAV length recovery after forced
termination; it is not evidence of normal finalization. A separate 5:03.392 take
on the deployed candidate finalized through Stop Recording at 53,518,380 bytes.
Both decoded fully with no exact-zero or -90 dB silence spans of at least 10 ms.
These are internal master measurements, not analog DDJ loopback captures.

For future regressions, record load timestamps and file growth, stop normally,
validate WAV frame counts and decode the entire take. Correlate gaps with source
silence and thread/kernel waits; lack of logged underruns alone is insufficient.
Some cold loads still took 5–6 seconds during concurrent deployment I/O without
breaking the BFQ recording. Test external SD readers separately; the final run
used PEN 2 with the problematic reader/hub disconnected.
