# Post-merge CI repair task

- [x] Base: freshly fetched `origin/codex/v0.0.7` at
  `170304b14a050758f7626f3f3dc86b87af090b72`.
- [x] Feature branch: `codex/post-merge-ci-check`.
- [x] Worktree: `/Users/pablofeijo/Documents/custom-bitedj-work/post-merge-ci-check`.
- [x] Intended merge target: `codex/v0.0.7`; user authorized squash merge and CI validation.
- [x] Incorporated the advanced integration tip `1ddf31df579eefdd2f7210b0e17a5c6978ff5159`
  before the squash; preserved its Docker environment and cleanup documentation.
- [x] Artifact version: not applicable; no product binary or deliverable image built.
- [x] Inspect [failed run 34324231801](https://github.com/pablo-feijo/custom-bitedj/actions/runs/34324231801):
  1,018 native, 42 removable store and 12 removable sampler tests passed.
  E2E runtime construction failed because its default ARM64 builder tag was
  absent, while the workflow had built `bitedj-builder:latest` on Ubuntu 24.04.
- [x] Pass that matching local builder tag explicitly to the GUI Docker build.
  Update the setup example and require post-merge CI checking and repair in both
  agent guides and [the testing procedure](TESTING.md#post-merge-ci-check-and-repair).
- [x] Fast controller/resource suite passes; workflow YAML parses;
  `git diff --check` passes.
- [x] Initial repair squash-published as `c0eb61e5116db0ec333edede9166156345b96ac7`.
  [Run 34342649328](https://github.com/pablo-feijo/custom-bitedj/actions/runs/34342649328)
  passed fast checks but was cancelled by the next integration push while compiling.
- [x] Incorporated `41c41ca62815331e78a1e4b7994f6e479ac3b9f6` for the cache follow-up.
- [x] Implement exact binary reuse, fresh asset replacement, original provenance,
  early compiler-cache saving, two build workers and an E2E runtime cache.
- [x] Fast cache regressions cover asset/docs hits, source/config/toolchain and
  embedded-resource invalidation, shared-library links and stale-asset removal.
- [ ] Validate the cache strategy remotely, including a second compatible run
  that skips compilation and still passes all native/removable/E2E checks.
