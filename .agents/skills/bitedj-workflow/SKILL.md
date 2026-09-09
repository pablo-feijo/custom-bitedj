---
name: bitedj-workflow
description: "Start or resume Custom Bite DJ repository changes with the agreed release base, isolated worktrees, architecture constraints and validation rules. Use for implementation and repository refactoring tasks."
---

# Task workflow

Required before making repository changes. Commands run from the task worktree root.

## Start or resume

1. Read [the active release record](../../../docs/BRANCH_VERSIONING.md). It owns the active
   integration branch, target version and release transition history; update it
   when the user advances the release instead of duplicating the value here.
2. Inspect `git status --short`, local branches and `git worktree list`. For a
   new task, fetch that exact remote integration ref and compare its tip with
   the local integration branch. Preserve local-only integration commits and
   other tasks; do not reset or switch their checkouts.
3. Create a separate `codex/<topic>` branch/worktree from the freshly fetched
   active remote tip, following [branch creation](../../../docs/BRANCH_VERSIONING.md#starting-work).
   For follow-ups, reuse the task worktree and record deliberate base updates.
4. Create an ignored `tasks/<topic>.md` checklist with the active release, base
   ref and resolved SHA, feature branch, worktree, merge target, intended checks,
   instance ownership when applicable and binary version (or “not built”).
5. Use the relevant repository skill from [Codex setup](../../../docs/CODEX.md).
   Read [architecture](references/architecture.md) before native/UI/controller
   edits and [UI workflow](../bitedj-ui/SKILL.md) before visible changes.
   Read additional guides only when the work enters their scope.

## Search and tool strategy

- Use `rg --files` to locate likely files, then `rg -n` for symbols/headings and
  bounded reads for the relevant sections. Prefer `git ls-files` when auditing
  tracked paths, and include hidden files when finding callers or moved links.
  Avoid dumping entire large guides or generated build trees into context.
- Prefer repository scripts and purpose-built APIs/CLIs over improvised shell
  sequences or GUI clicks. Follow [build/test tools](../bitedj-build-test/references/tools.md) before
  builds, container operations, screenshots or preview delivery.
- Batch independent read-only searches when the tools support it. Keep edits,
  ownership checks and dependent commands sequential. Inspect results before
  using discovered paths, container names, ports or coordinates.
- Use a real browser to verify a noVNC desktop after the JavaScript checks;
  HTTP success alone is insufficient. Use exact layout or screenshot evidence
  for GUI coordinates. Read only the applicable UI map from the index.
- Keep scratch helpers, execution notes and generated output in ignored task
  runtime directories. Follow [repository layout](../../../docs/REPOSITORY_LAYOUT.md) for
  reusable scripts, fixtures and docs; do not accumulate files at the root.

## Validate and finish

- Pick the relevant [test layer](../../../docs/TESTING.md#run-the-appropriate-layer).
  Documentation-only edits need link/anchor and diff checks. Asset reuse must
  follow the [compiled-output cache protocol](../../../docs/TESTING.md#reuse-compiled-outputs-for-asset-changes)
  and still rerun applicable tests. New deliverables need rebuilt, verified
  [branch-specific versions and provenance](../../../docs/BRANCH_VERSIONING.md#every-work-branch-has-an-identifiable-binary).
- Keep build/install/settings/results directories independent per worktree.
  Preserve requested previews; report branch, instance, actual endpoints and
  validation results when delivering one.
- Before committing, review the diff, run `git diff --check`, verify configured
  author/committer identity and run `python3 scripts/test/check-git-storage.py`
  after staging. Follow [storage policy](../../../docs/GIT_STORAGE.md): 1 MiB staged blob
  limit; reduce larger assets or keep them outside Git. Do not configure Git
  LFS in these forks. Generated artifacts remain ignored.
- Use Conventional Commits: `type(scope): description` (scope optional), with
  an appropriate type, breaking-change marker/footer when applicable and source
  attribution for adapted work. Check every commit created for this task and
  amend nonconforming messages without rewriting unrelated/published history.
- Merge into the agreed semver branch only when requested, with one squash
  Conventional Commit for the complete task. Do not bring intermediate commits
  or a merge commit into semver history. Hardware deployment also requires a
  user request; preserve authorization already given in the task.
- Rewrite published semver history only when explicitly requested. Preserve
  unrelated history and a local backup ref; use an explicit expected-value
  force-with-lease when publishing a rewrite.
- After every authorized merge and push, record the exact published SHA and
  run links, then follow [post-merge CI and repair](../../../docs/TESTING.md#post-merge-ci-check-and-repair)
  through all expected jobs including desktop E2E. Investigate and fix failures,
  validate/publish repairs within authorization and recheck the resulting SHA.
  Pending, cancelled, missing or externally blocked checks remain unresolved.
- Finish authorized integration with the [cleanup audit](../bitedj-integration/references/cleanup.md), including
  actual sibling folders, ignored artifacts, nested repositories and Docker
  mounts. Report deferred cleanup with specific reasons and next actions.
- Keep findings in the owning guide and activity logs in ignored `tasks/`.
  Report the result, validation, remaining limitations and integration status.
