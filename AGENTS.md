# Agent instructions

Custom Bite DJ is an independent fork of Team Deckshark's BiteDJ, based on
Mixxx. Preserve upstream attribution, licenses and technical identifiers.

## Start here

1. Read [the task workflow](docs/agents/workflow.md) before changes.
2. Use [the agent tooling index](docs/agents/README.md) to load only the guides
   relevant to this task. Read another guide when the task crosses its trigger.
3. Fetch the active base in [branch versioning](docs/BRANCH_VERSIONING.md), inspect
   local integration commits, and create a separate `codex/<topic>` worktree.
   Reuse it for follow-ups; preserve other checkouts and live GUI instances.

## Rules for every task

- Keep build outputs, settings, containers and ignored `tasks/<topic>.md`
  checklists local to the task worktree. Follow [repository layout](docs/REPOSITORY_LAYOUT.md).
- Use Conventional Commits. Integrate only when requested, with one squash
  commit per task; do not rewrite published history without explicit authorization.
- Before building, follow [build and test tooling](docs/agents/build-and-test.md).
  Deliverables must embed and report the full branch-specific version; never
  relabel old binaries. Run the checks appropriate to the changed inputs.
- Before UI/native/controller changes, read [architecture](docs/agents/architecture.md)
  and [UI workflow](docs/agents/ui-workflow.md), then the relevant control map.
  Preserve saved control IDs; update affected maps, automation and gallery images.
- Before manual GUI commands, source `scripts/test/gui-test-settings.sh` and run
  `verify_test_instance_owner`. Use the owned `$CONTAINER_NAME` and discovered
  ports. Pass the [noVNC gate](docs/agents/build-and-test.md#novnc-delivery-gate)
  before delivering a preview URL.
- After an authorized merge and push, follow [exact-commit CI and repair](docs/TESTING.md#post-merge-ci-check-and-repair)
  through all expected jobs, then [audit cleanup](docs/agents/cleanup.md).
- Follow [Git storage and identity](docs/GIT_STORAGE.md) before committing.
  Generated output stays ignored; preserve unique user data and active work.

## Keep instructions small

Keep this file under 60 lines and `docs/AGENTS.md` as a short redirect. Put new
agent procedures in `docs/agents/`, linked by task trigger from its index.
Update the owning guide instead of appending feature history here. Keep detailed
recipes in their existing canonical documents, and execution logs in ignored
`tasks/`. See [guide maintenance](docs/agents/README.md#maintaining-these-guides).
