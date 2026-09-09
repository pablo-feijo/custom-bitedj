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
- [x] Follow-up build tuning: Ninja and up to four compiler workers, bounded by
  actual CPU count and 3 GiB RAM per worker. Keep existing compile flags and
  serial native tests. User prioritized immediate publication over waiting for
  the previous run; other active work is asked to hold integration pushes.
- [x] Replace Node 20 Actions with verified Node 24 releases: checkout/setup-node
  v7, cache v6, upload-artifact v7. Register a local GCC matcher without a Node action.
- [x] Priority squash published as `8f428a87728209cc3658be6f4ce58dab21b8d118`.
- [x] [Cold validation run 34347232331](https://github.com/pablo-feijo/custom-bitedj/actions/runs/34347232331)
  passed: 1,035 native tests, 42 store tests, 12 sampler tests and five desktop
  E2E tests, plus the fast suite. The Node 20 Action warning is absent.
- [x] Logs confirm four compiler workers. Native compilation took 26m 36s with
  939 compiler-cache misses; compiler, binary and runtime caches were all saved.
- [x] The original Docker builder-image mismatch is resolved by successful E2E.
- [ ] This documentation-only follow-up must restore the exact binary/runtime
  caches, skip both compilation commands and pass all test layers. Its CI run
  is the warm validation; record the final URL and timing in the task result.
