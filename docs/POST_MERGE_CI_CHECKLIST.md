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
- [ ] Integration/publication and remote validation of this repair remain pending.
- [ ] [Run 34340773525](https://github.com/pablo-feijo/custom-bitedj/actions/runs/34340773525)
  for the base SHA was still in progress when checked on 2026-09-09, with fast
  tests successful and native compilation running. It does not contain this repair.
