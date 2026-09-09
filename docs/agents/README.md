# Agent tooling and navigation

Read [workflow.md](workflow.md) for every change, then choose the applicable rows
below. These are repository procedures for any agent, not a tool-specific plugin.
Commands use repository-relative paths and run from the task worktree root unless
the owning guide says otherwise. Do not load every linked document by default.

## Choose the next guide and tool

| Task or next action | Read before acting | Tools / entry points |
| --- | --- | --- |
| Start or resume changes | [Workflow](workflow.md), [active branch record](../BRANCH_VERSIONING.md) | `git status`, `git worktree list`, targeted `git fetch`, `git worktree add` for a new task |
| Find code or instructions | [Search strategy](workflow.md#search-and-tool-strategy) | `rg --files`, `rg -n`, bounded file reads, `git diff` |
| Edit docs or move files | [Repository layout](../REPOSITORY_LAYOUT.md), [maintenance below](#maintaining-these-guides) | Search tracked/hidden references; check relative links and heading anchors |
| Change native code, UI or controller behavior | [Architecture](architecture.md) | Targeted source reads; [test layers](../TESTING.md#run-the-appropriate-layer) |
| Change any visible UI | [UI workflow](ui-workflow.md) plus the relevant map below | Owned 1024×600 GUI; capture affected gallery images |
| Settings or Browse | [Settings map](ui-settings.md) | [GUI procedures](../GUI_TESTING.md#6-headless-debugging--precision-ui-coordinate-guide) |
| Waveforms, Grid or Key | [Play map](ui-play.md) | [Waveform regressions](../TESTING.md#preview-and-waveform-regressions) |
| Beat FX or picker | [Effects map](ui-effects.md), [catalogue](../BEAT_FX.md) | `scripts/build/generate-beatfx.py`, `tests/effects/`, `src/test/` |
| Pad FX, touch drawer or DDJ-400 | [Pad map](ui-pads.md), [controller mapping](../DDJ400_MAPPING.md) | [Pad FX testing](../PAD_FX_TESTING.md), [touch regression](../GUI_TESTING.md#touchable-performance-pad-regression) |
| Build, launch GUI, capture or deliver a preview | [Build/test tools](build-and-test.md) | Build script → owned GUI launcher → appropriate tests → noVNC browser gate |
| Deploy to Pi or create/flash an image | [Build/test tools](build-and-test.md#versioned-images-and-reproducible-hardware-fixes), [deployment guide](../BUILD_AND_DEPLOY.md) | `scripts/deploy/`, `scripts/build/generate-pi-image.sh`; follow existing authorization |
| Commit, merge or publish | [Workflow completion](workflow.md#validate-and-finish), [storage/identity](../GIT_STORAGE.md) | `git diff`, staged size guard, squash integration when requested, exact-SHA CI checks |
| Retire worktrees, branches or containers | [Cleanup](cleanup.md), [Docker maintenance](../DOCKER_MAINTENANCE.md) | Audit refs, actual folders, ownership labels and mounts before scoped removal |

## Maintaining these guides

- Keep root `AGENTS.md` under 60 lines; `docs/AGENTS.md` stays a redirect under 15.
  Keep this index under 80 lines and each topic guide under 150 lines.
  Split by task trigger when a guide outgrows that budget, and update this table.
- Keep one owner for each detailed rule or control map. Existing public guides
  such as TESTING, GUI_TESTING, BRANCH_VERSIONING and LICENSING remain canonical
  for their procedures. Link to their exact sections instead of copying recipes.
- Put agent-specific workflow and control constraints here. Use descriptive `.md`
  filenames; do not add more automatically loaded `AGENTS.md` files for topics.
- Update the relevant map and affected GUI procedure together for a UI change.
  Replace superseded coordinates in place, preserve control/value mappings and
  released screenshot galleries, and identify capture-dependent geometry.
- When moving a section, search tracked files including hidden CI/config paths,
  update inbound links and relative outbound links, and verify heading anchors.
  Check nested repositories separately if they reference a moved guide; do not
  modify unrelated submodule content. Avoid redirect chains between topic guides.
- Add durable findings to the owning guide, not a chronological appendix. Keep
  plans, commands run, results and migration checklists in ignored `tasks/`;
  raw artifacts belong in ignored `test-results/`. No published links to local logs.
- For a docs-only change, verify links/anchors, `git diff --check`, the budgets
  above and the staged storage guard. A build or screenshot refresh is required
  only when the corresponding application/UI inputs changed.
