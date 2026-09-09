# BiteDJ folder review — 2026-09-09

Reviewed actual directories under `/Users/pablofeijo/Documents`, not just registered
parent-repository worktrees. The first inventory contained 20 BiteDJ-related folders.
Five more folders were retired in this pass. Earlier passes retired three parent
worktrees plus the UI-docs nested submodule worktree.

## Removed in this pass

| Folder | Evidence and preservation |
| --- | --- |
| `bitedj-docs-licensing` | Published `319a6b13c6` differs only by six added integration-checklist lines. Original detached commit retained under a backup branch. |
| `bitedj-test-suite` | All changed implementation/test files match published `3703aeb9d3`; differences are the subsequent agent/versioning guides. Original branch and backup ref retained. |
| `bitedj-ddj400-triple-shift` | Controller code/tests match published `7a3efd0b64`; reviewed differences are integration version, newer waveform sources/docs and integration checklist. Original branch and backup ref retained. |
| `bitedj-pigen-v007-merge` | Clean duplicate Pi-gen checkout at `ee05117`, identical to the primary Pi-gen checkout. Nested repository refs and HEAD preserved in a verified full Git bundle. |
| `bitedj-next-batch` | Entire tree, including gitlink, matches published `d502d02a3c`. Original branch and backup ref retained. Removing the duplicate Pi-gen checkout released its dependency. |

No unique branch history was deleted on the strength of squash equivalence.
These were inactive, clean checkout removals with source history retained. Docker
labels and mount sources were checked immediately before removal. All removals
used non-forced Git commands. The next-batch nested Git metadata was moved into
recovery after deinitialization because Git otherwise refuses removal of a worktree
containing stored submodule repositories.

Recovery root:
`/Users/pablofeijo/Documents/bitedj/test-results/worktree-cleanup/2026-09-09-folder-review/`.
Per-folder manifests describe moved local outputs. `next-batch-pigen-all.bundle`
contains all nested refs and HEAD and passed `git bundle verify`; raw nested Git
metadata is also preserved. Earlier recovery files remain under the adjacent
`2026-09-09-squash-worktrees/` directory. Moving outputs does not reclaim their disk
space. They are preserved pending a separate decision to discard old artifacts.

## Remaining folders

Sizes are approximate from the initial inventory, before ongoing builds and
recovery moves. Main-repository usage increased when runtime outputs were moved
into its ignored recovery directory.

| Folder | Initial size | Reason retained |
| --- | ---: | --- |
| `bitedj` | 28 GB | Primary Git repository; existing tracked/submodule modifications and local files. Also holds cleanup recovery artifacts. |
| `bitedj-grid-deck-controls` | 646 MB | Active task, uncommitted source changes and running build mount. |
| `bitedj-xsploit-jog-beatfx` | 166 MB | Newly created task worktree during the audit; not an old completed task. |
| `bitedj-touch-pad-mode-flash` | 5.7 GB | Active edits, running build and preview. |
| `bitedj-semver-vnc` | 5.6 GB | Current integration preview. |
| `bitedj-browse-fx-touch-ui` | 5.6 GB | Running older task preview; retire its container before removing backing files. |
| `bitedj-waveform-fixes` | 5.6 GB | Running older task preview; retire its container before removing backing files. |
| `bitedj-controller-pad-drawer` | 6.1 GB | Running older task preview; retire its container before removing backing files. |
| `bitedj-ddj400-fx-back` | 1.6 GB | Running older task preview; retire its container before removing backing files. |
| `bitedj-v007-integration` | 166 MB | Retained active integration branch checkout. |
| `bitedj-docker-branch-env` | 166 MB | This cleanup/documentation task; unmerged commits, reusable for follow-ups. |
| `bitedj-fork-review` | 5.4 GB | Inactive and clean, but reviewed source changes differ in library/Rekordbox implementation; full integration is not established. Preserve source and outputs. |
| `custom-bitedj-work` | 5.6 GB | Independent Git repository with a modified submodule and untracked files. Not a parent-repository worktree. |
| `bitedj-custom-mixxx-v0.0.1` | 2.2 GB | Packaged binary/resources and historical OS image, not a worktree. |
| `BiteDJ_OS_Builds` | 24 GB | Historical 0.0.3/0.0.4/0.0.5 build archives, not worktrees. |

The four older running preview folders above account for about 19 GB. Release
artifact folders account for about 26 GB. Neither category should be confused
with an inactive, redundant checkout. This review did not stop previews, discard
release archives, alter active tasks or delete remote branches.
