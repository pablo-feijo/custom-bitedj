# Integration cleanup

[Agent tooling index](README.md). Read this guide when its task trigger applies.

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

- Finish every authorized integration with a cleanup audit, including the actual
  sibling folders on disk, Git worktrees, Docker labels and bind mounts, nested
  repositories, and ignored artifacts. Do not stop at `git branch --merged`:
  squash merges require checking the integrated source changes.
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

For authorized cleanup of older branch containers, follow [the audited preview
cleanup procedure](../DOCKER_MAINTENANCE.md#older-branch-previews-and-running-containers).
Check host and Docker disk usage, retire obsolete previews by exact ID, and keep
current review instances, active builds, bind-mounted data and named caches.

Cleanup also retires completed `tasks/` plans after durable findings reach the
guides; preserve active work and consolidate unresolved backlog items. Never
commit task plans or backups. Do not archive completed task folders indefinitely.
