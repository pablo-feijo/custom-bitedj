# Docker branch environment and worktree cleanup — 2026-09-09

- [x] Fetch base `origin/codex/v0.0.7`.
- Base commit: `9bf0593966ef64945b96582e89f6d319b124d34c`.
- Feature branch: `codex/docker-branch-env`.
- Worktree: `/Users/pablofeijo/Documents/bitedj-docker-branch-env`.
- Intended merge target: `codex/v0.0.7`; no integration or publication requested.
- Binary version: not applicable; documentation-only task, no artifacts built.
- [x] Confirm launcher already consumes task-scoped environment variables.
- [x] Document exports, discovered browser URLs, override precedence and lack of
  automatic `.env` loading in GUI_TESTING.md and both agent instruction files.
- [x] Verify shell syntax, `git diff --check`, and read-only ownership/port discovery
  against an existing GUI instance. No GUI was replaced or opened for this task.
- [x] Inspect all worktrees, statuses, ignored files, submodule state for the removal
  candidate, active tasks, Docker labels and bind mounts.
- [x] Remove inactive clean `bitedj-controller-pad-release` and its local branch
  `codex/controller-pad-release` using non-forced Git commands. Its tip
  `c3b314ff54739b01b767477bbe9f4f5f0160e248` is an ancestor of the fetched base.
  Its submodule was uninitialized; its only ignored files were three Python caches.
  Preserve those in `/Users/pablofeijo/Documents/bitedj/test-results/worktree-cleanup/2026-09-09-controller-pad-release-caches.tar.gz`.
- [x] Verify removal and preserve all other worktrees and remote branches.

## Deferred cleanup

- Main checkout: release branch, tracked/submodule changes and local files.
- `bitedj-v007-integration`: retained integration branch with a different local tip.
- `bitedj-touch-pad-mode-flash`: merged tip, but active edits and running preview.
- `bitedj-browse-fx-touch-ui`, `bitedj-controller-pad-drawer`,
  `bitedj-ddj400-fx-back`, `bitedj-waveform-fixes`: running previews and no tip
  ancestry proof to retained release refs.
- `bitedj-semver-vnc`: active task, running preview and an unlabeled build container
  bind-mounting the worktree; no tip ancestry proof.
- `bitedj-ddj400-triple-shift`, `bitedj-docs-licensing`, `bitedj-fork-review`,
  `bitedj-next-batch`, `bitedj-test-suite`, `bitedj-ui-docs`,
  `bitedj-v007-pigen-settings`: no tip ancestry proof to retained release refs.
  Some also hold generated outputs. Squash-equivalent changes do not satisfy
  the repository's ancestry requirement; preserve these tips.
- `bitedj-docker-branch-env`: current task worktree, retained for follow-ups.

See [the launch recipe](GUI_TESTING.md#environment-variables-for-launching-and-opening-a-branch-preview)
and [cleanup policy](AGENTS.md#branch-cleanup-after-integration).

## Follow-up cleanup

The repeated request to remove already-merged checkout copies was audited against
published `origin/codex/v0.0.7` at `170304b14a050758f7626f3f3dc86b87af090b72`.
For rewritten integration history, exact Git tree identity (including gitlinks)
was verified while preserving original commits under backup refs. This removes
redundant checkout files without deleting unique commit history or claiming that
squash equivalence establishes ancestry for branch deletion.

- Removed `/Users/pablofeijo/Documents/bitedj-ui-docs`: source tree identical to
  published `5d05a962afb8b38a9ebc9ff65bebb95ab9e6adfd`. Original commit retained at
  `backup/worktree-cleanup-20260909-ui-docs`. Its clean nested submodule worktree
  was removed with Git; its branch remains. Builds, settings, music and results
  were moved to `/Users/pablofeijo/Documents/bitedj/test-results/worktree-cleanup/2026-09-09-squash-worktrees/bitedj-ui-docs/`.
  This directory also contains a source manifest and submodule recovery bundle.
- Removed `/Users/pablofeijo/Documents/bitedj-v007-pigen-settings`: source tree
  identical to published `89862f1f6c5d5eb46567256f160819d863fac156`. Original commit
  retained at `backup/worktree-cleanup-20260909-v007-pigen-settings`. No ignored
  outputs or initialized submodule needed preservation.
- Retained `bitedj-next-batch` despite exact tree identity with published
  `d502d02a3cbd4fc6e80d00a902ba170512cb4663`: its embedded `mixxx-pi-gen/.git`
  owns the additional `/Users/pablofeijo/Documents/bitedj-pigen-v007-merge`
  worktree. Git refused submodule deinitialization because relocation with
  multiple worktrees is unsupported. No force removal was used. A backup ref
  and submodule bundle were created; its original files remain in place.
- Running previews, active changes, the integration checkout and remaining
  uncertain histories remain untouched. No remote branches were deleted.

Moving runtime outputs into recovery preserves data; it does not reclaim their
 disk usage. Retire recovery artifacts separately when they are no longer needed.

## Full filesystem review follow-up

See [the folder-by-folder audit](FOLDER_CLEANUP_REVIEW.md). It supersedes the earlier
next-batch dependency deferral: the duplicate Pi-gen checkout was safely retired,
its complete Git history bundled, and next-batch removed with all local outputs
preserved. Licensing, test-suite and triple-Shift checkout copies were also removed
following review of their published integration differences. Original refs remain.


## Authorized squash integration and final cleanup

- User requested squash integration into `codex/v0.0.7` and stronger cleanup rules.
- Integration starting tip: `170304b14a050758f7626f3f3dc86b87af090b72`.
- Agent rules now require actual folder audits, evidence for squash integration,
  retirement of completed checkouts, disposal of identified temporary outputs,
  preservation of unique files, and explicit accounting for remaining folders.
- Validation: documentation diff checks and launcher shell syntax; no application
  code or binary changed, so no rebuild is required.
- Local integration only; retain the feature ref after removing this task's clean
  checkout. Publication and remote-ref cleanup remain separate.
