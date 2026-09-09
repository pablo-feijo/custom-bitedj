# Agent instructions

Custom Bite DJ is an independent fork of Team Deckshark's BiteDJ, based on
Mixxx. Preserve upstream attribution, licenses and technical identifiers.

## Start here

1. Use `$bitedj-workflow`: read [the task workflow](.agents/skills/bitedj-workflow/SKILL.md) before changes.
2. Use [the native skills guide](docs/CODEX.md) to load only the guides
   relevant to this task. Read another guide when the task crosses its trigger.
3. Fetch the active base in [branch versioning](docs/BRANCH_VERSIONING.md), inspect
   local integration commits, and create a separate `codex/<topic>` worktree.
   Reuse it for follow-ups; preserve other checkouts and live GUI instances.

## Rules for every task

- Keep build outputs, settings, containers and ignored `tasks/<topic>.md`
  checklists local to the task worktree. Follow [repository layout](docs/REPOSITORY_LAYOUT.md).
- Use Conventional Commits. Integrate only when requested, with one squash
  commit per task; do not rewrite published history without explicit authorization.
- Before building, use [$bitedj-build-test](.agents/skills/bitedj-build-test/SKILL.md).
  Deliverables must embed and report the full branch-specific version; never
  relabel old binaries. Run the checks appropriate to the changed inputs.
- Before UI/native/controller changes, read [architecture](.agents/skills/bitedj-workflow/references/architecture.md)
  and [$bitedj-ui](.agents/skills/bitedj-ui/SKILL.md), then the relevant control map.
  Preserve saved control IDs; update affected maps, automation and gallery images.
- Before manual GUI commands, source `scripts/test/gui-test-settings.sh` and run
  `verify_test_instance_owner`. Use the owned `$CONTAINER_NAME` and discovered
  ports. Pass the [noVNC gate](.agents/skills/bitedj-build-test/references/tools.md#novnc-delivery-gate)
  before delivering a preview URL.
- After an authorized merge and push, follow [exact-commit CI and repair](docs/TESTING.md#post-merge-ci-check-and-repair)
  through all expected jobs using [$bitedj-integration](.agents/skills/bitedj-integration/SKILL.md), including cleanup.
- Follow [Git storage and identity](docs/GIT_STORAGE.md) before committing.
  Generated output stays ignored; preserve unique user data and active work.

## Keep instructions small

Keep this file under 60 lines and `docs/AGENTS.md` as a short redirect. Put new
agent procedures in `.agents/skills/<name>/SKILL.md` with precise triggers.
Put conditional detail in each skill’s `references/`, not in this entry point. Keep detailed
recipes in their existing canonical documents, and execution logs in ignored
`tasks/`. See [guide maintenance](docs/CODEX.md#maintaining-these-guides).
