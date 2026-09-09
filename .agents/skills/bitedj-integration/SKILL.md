---
name: bitedj-integration
description: Squash-integrate Custom Bite DJ work, follow published SemVer CI and audit completed worktree, branch or container cleanup. Use for integration, publication follow-through or cleanup requests; preserve existing authorization scope.
---

# Integrate and retire Custom Bite DJ tasks

1. Confirm the user's authorized scope and the merge target in
   [the active release record](../../../docs/BRANCH_VERSIONING.md). Inspect both
   feature and integration checkouts and preserve unrelated changes.
2. Validate the complete task diff with the relevant tests and staged storage
   guard. Integrate with one squash Conventional Commit; do not rewrite published
   history unless explicitly requested. Follow [completion rules](../bitedj-workflow/SKILL.md#validate-and-finish).
3. After authorized publication, follow [post-merge CI and repair](../../../docs/TESTING.md#post-merge-ci-check-and-repair)
   for the exact published SHA through fast, native, removable-drive and desktop
   E2E checks. Read failed logs, fix deterministic failures in an isolated task
   worktree and recheck the published repair. Follow replacement runs after a
   newer push; missing, cancelled or externally blocked checks remain unresolved.
4. Read [the cleanup audit](references/cleanup.md) before removing anything.
   Check exact squash changes, refs, actual directories, ignored artifacts,
   nested repositories and Docker mounts. Preserve source recovery, live previews,
   unique data and other active tasks. Use non-forced worktree removal.
5. Report the integration SHA, run links/results, removed task artifacts and
   remaining items with specific reasons. Retire completed local execution plans.

Read [Docker maintenance](../../../docs/DOCKER_MAINTENANCE.md) only for container
or disk cleanup; global pruning requires the user's explicit cleanup scope.
