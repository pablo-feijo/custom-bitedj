# Build workflow review

Reviewed 2026-09-09. The initial assessment below is followed by the implementation status. Local
build scripts have now been updated; CI configuration remains unchanged. Commit metadata does not identify
which assistant authored the changes, so attribution to Gemini is unverified.

## Task checklist

- [x] Fetch base `origin/codex/v0.0.7`, resolved to
  `51da207bdd` (the freshly fetched integration tip).
- [x] Create feature branch `codex/build-workflow-review` in
  `/Users/pablofeijo/Documents/bitedj-build-workflow-review`.
- [x] Preserve existing integration checkout and its local state.
- [x] Record merge target: `codex/v0.0.7`; no merge or push performed.
- [x] Review CI acceleration commits `6febdd5680` and `8f428a8772`,
  cache helper, local Docker build, and image-generator entry point.
- [x] Verify existing cold and warm CI run conclusions and step durations via GitHub API.
- [x] Run cache regression tests: three passed.
- [x] Record broader validation limit: fast suite could not start because
  `node` is absent from this shell's PATH. No native build or Pi image generated.
- [x] Binary version: not applicable to this review; no deliverable built.

## Verified baseline

Keep exact compiled-output caching, replacement of runtime assets (including
deleted files), original binary provenance, early compiler-cache saving, Ninja,
and memory-aware CI parallelism. The cache tests cover key invalidation and
restoration, including embedded Qt resources and shared-library symlinks.

Both the [cold run](https://github.com/pablo-feijo/custom-bitedj/actions/runs/34347232331)
and [warm run](https://github.com/pablo-feijo/custom-bitedj/actions/runs/34350706794)
completed successfully. The existing checklist records total duration falling
from 36m 42s to 5m 42s. API step timings confirm:

| Step | Cold | Warm |
| --- | ---: | ---: |
| Checkout | 98s | 30s |
| Install build dependencies | 51s | 66s |
| Configure | 12s | 6s |
| Build native test suite | 1,596s | Skipped |
| Build desktop/install/snapshot | 11s | Skipped |
| Restore exact binaries | Miss | 5s |
| Native tests | 125s | 108s |
| Removable integration tests | 13s | 13s |
| Restore runtime archive | Miss | 9s |
| Build or load GUI runtime | 160s | 46s (load) |
| Desktop E2E | 36s | 32s |

These are observations from two runs, not a benchmark for future changes.

## Priorities

1. **Validate artifacts before OS assembly (correctness).**
   `scripts/build/generate-pi-image.sh:20` skips compilation whenever either
   executable exists. It never validates ARM64 architecture, current compiled
   inputs, binary version or asset freshness. A previous AMD64 test build or
   stale ARM64 output can therefore be passed to pi-gen. Add a build manifest
   and validate architecture, source/input identity, product version and assets
   before assembly. Rebuild mismatches and check the actual binary version in
   the matching container. The build guide currently overstates this step as
   automatically compiling the binary on every invocation.

2. **Make local incremental builds deterministic (correctness and speed).**
   `scripts/build/docker-build.sh:81` reuses a builder merely because its tag
   exists; Dockerfile or dependency-script changes do not trigger refresh.
   The same worktree also uses one build/install directory for all architectures.
   Key the immutable builder by platform and recipe inputs, and validate or
   separate build directories by platform/toolchain. Use Docker layer caching
   when checking/building the recipe. Preserve worktree ownership and the shared
   compiler cache.

3. **Bring CI's build controls to the local helper (speed and reliability).**
   `scripts/build/docker-build.sh:99` does not select Ninja, and lines 125/137
   use every reported CPU without considering available memory. Add explicit
   generator selection, a user-overridable memory-aware worker limit, and cache
   statistics. Handle existing Makefile build directories explicitly: changing
   generator in place is not a valid migration. Account for container memory
   limits when selecting workers. Measure cold, one-source-file incremental,
   and asset-only builds separately.

4. **Stabilize the CI environment before narrowing fingerprints (cache hit rate).**
   `.github/workflows/tests.yml:89` hashes every installed runner package, and
   `scripts/test/ci-build-cache.py:35` includes nearly every non-documentation,
   non-runtime-asset repository file. Unrelated package or test-helper changes
   can invalidate otherwise compatible binaries. First establish a pinned build
   environment and explicit compiled-input contract; retain all compiler,
   headers, linked libraries, configuration and embedded-resource dependencies.
   Do not loosen the key without invalidation tests. A pinned dependency image
   may also reduce the observed 66s warm dependency installation, but its download
   cost must be measured.

5. **Reduce GUI runtime transfer size (warm CI speed).**
   `docker/gui-test.Dockerfile:2` inherits the entire builder; the cached archive
   therefore includes compilers and development dependencies. Restoring/loading
   it cost 55s on the warm run. Build a runtime-only image from the compatible
   OS baseline with required libraries and GUI/audio tools, then measure image
   size and restore/load time. Retain the existing E2E checks to catch missing
   runtime libraries. CI Ubuntu and deliverable Pi Debian must remain compatible
   with their respective binaries.

## Proposed build sequence

1. Select branch, target platform and prerelease version; record provenance.
2. Resolve a matching cached builder and platform-specific build directory.
3. Reuse exact compatible CI binaries for asset-only changes; otherwise perform
   an incremental Ninja/ccache build with bounded workers.
4. Refresh installed assets and verify executable architecture/version.
5. Run fast checks, affected native tests and required integration/E2E checks.
6. Hot-deploy for application iteration when requested. Generate an OS image
   when requested for OS changes or distribution, using validated artifacts.

Implement priorities 1–3 together before further CI cache tuning. Keep full
release version/provenance checks separate from CI's permitted reuse of older
compatible binaries. No new speedup estimate is justified until measured.

## Implementation follow-up

- [x] Reuse `codex/build-workflow-review` and its original base for this follow-up.
- [x] Implement priorities 1–3: source/install manifest, ELF and binary-version
  validation, dirty-source snapshot, verified staging, image-version gate,
  Docker recipe refresh, toolchain/platform/generator guard, Ninja, cgroup-aware
  workers, optional worker override and compiler-cache statistics.
- [x] Keep output paths compatible with existing GUI/deploy consumers; preserve
  prior installed artifacts on build/test/verification failure.
- [x] Test provenance failures and mocked build/image orchestration, including
  outside-repository invocation, image-version mismatch and legacy build guards.
- [x] Verify real Docker worktree Git resolution and worker calculation (2 workers
  on the available ARM64 builder).
- [x] Run full fast suite using bundled Node; the earlier PATH limitation is resolved.
- [x] Update build and layout documentation. No binary/image delivery, native
  compilation, integration merge or push performed in this script-only task.
- [ ] Priorities 4–5 remain separate CI experiments: pin the environment and
  measure runtime image alternatives before changing the proven CI cache path.

Validation uses synthetic ELF fixtures and a mocked Docker build for orchestration;
these are not claimed as a real application build. No new speedup is claimed.

## Authorized validation and integration

- User authorized real validation, squash merge into `codex/v0.0.7`, push and
  post-push CI checking on 2026-09-09.
- Incorporated remote/local integration tip `e0095253311308b7525bd7062b466f13eddc64a1`.
- Local ARM64 validation binary: `0.0.7-codex-build-workflow-review.1`.
- Integration version will use a fresh `codex-v0-0-7` build number; feature
  binaries retain their original version and provenance.

- Pre-push validation: full fast suite passed (18 Python fast tests plus the
  controller, Pad FX, effect-catalog and noVNC checks); shell syntax and diff
  checks passed. Previous integration commit CI completed successfully.
- Squash integration uses `0.0.7-codex-v0-0-7.7`. The ARM64 branch build and
  exact published-commit CI remain pending until their completion is recorded.

- Published squash `c9d915cc187c2c539677808bcdc8e689247c9493` passed
  [CI run 34355902985](https://github.com/pablo-feijo/custom-bitedj/actions/runs/34355902985):
  1,043 native, 42 removable store, 12 removable sampler and five desktop tests.
- User additionally authorized removal of older branch Docker containers and
  requested the cleanup guide update. Retired six audited obsolete previews,
  removing 377,073,664 writable-layer bytes. Kept current SemVer/pad-touch
  previews, active builds, bind-mounted data and the named compiler cache.
- Actual unchanged-recipe retry exposed BuildKit attestation churn: different
  OCI image IDs had identical rootfs/config fingerprints. Cache compatibility
  now uses the latter, with a verified migration of the initial image-ID marker.
- Validation build `.1` was intentionally interrupted to resume after preview
  cleanup with four workers. Builder-marker rejection occurred before further
  compilation; no binary was published. The repaired build uses
  `0.0.7-codex-build-workflow-review.2` and reuses proven-compatible objects.

- Repair validation: 19 Python fast tests and the controller/effect/noVNC checks
  pass. A real Docker retry migrated the old marker and resumed compatible
  objects successfully with four workers. Integration repair version is
  `0.0.7-codex-v0-0-7.8`; final ARM64 install and post-repair CI remain pending.
