# Branch bases and binary versioning

## Active unreleased release record

| Field | Current value |
| --- | --- |
| Status | Unreleased, active development |
| Target product SemVer | `0.0.7` |
| Integration branch | `codex/v0.0.7` |
| Remote | `origin` |
| Last verified remote tip | `559edc3def` on 2026-09-09 |

The commit above is an audit record, not a pinned starting point. Fetch the active
branch before each new task and record the actual resolved commit. Do not start
from an older release, a stale local integration checkout, or an arbitrary branch
with a larger version number. The latest **active unreleased** branch is the one
agreed with the user. When a newer release becomes active, update this table,
`agents.md` and `docs/AGENTS.md` together; retain the transition history below.

## Starting work

1. Read this record; inspect the working tree, branches and existing worktrees.
2. Fetch `origin` and verify the active integration branch still exists and is
   unreleased. Compare available SemVer branches numerically, not lexicographically.
   If the record is superseded by an agreed new target, update it before proceeding.
3. Create a separate `codex/<topic>` feature branch and worktree from the freshly
   fetched remote tip. Preserve other tasks and their uncommitted changes.
4. Record the active release, remote base, full resolved commit, feature branch,
   worktree, binary version and intended merge target in the task checklist.
5. Reuse that worktree for follow-ups. Do not restart a task because the release
   branch advanced. Integrate later changes deliberately and record the new base.

Example for the current active release:

```bash
git fetch origin codex/v0.0.7
git rev-parse origin/codex/v0.0.7
git worktree add -b codex/my-feature ../bitedj-my-feature origin/codex/v0.0.7
```

## Every work branch has an identifiable binary

Use `MAJOR.MINOR.PATCH-<branch-slug>.<build-number>` for development binaries.
Keep the agreed release core; do not bump each feature to a different release
when it is explicitly targeting the same unreleased integration version.
Normalize the **full branch name** to lowercase ASCII letters, digits and hyphens:
replace `/`, `.`, `_` and other separators with `-`, collapse repeated hyphens and
trim them. Prefix the slug with `branch-` if it would otherwise be purely numeric.
Record the original branch name in provenance to disambiguate normalized names.
The build number is a positive decimal integer without leading zeros; increment
it for every new deliverable build on that branch. Never overwrite a previous
artifact with changed contents under the same version.

Example: branch `codex/controller-pad-drawer`, target `0.0.7`, first build:

```cmake
set(BITEDJ_VERSION "0.0.7")
set(BITEDJ_VERSION_PRERELEASE "codex-controller-pad-drawer.1")
```

The binary must report `0.0.7-codex-controller-pad-drawer.1`. This is part of the
compiled product version, not merely an archive name. Integration branch test
builds also need a branch suffix, for example `0.0.7-codex-v0-0-7.1`. Remove the
suffix only when preparing the explicitly authorized final `0.0.7` release.
Keep the upstream Mixxx base version independent.

Before each deliverable build:

- Update the CMake product version and prerelease suffix in the work branch.
- Synchronize package metadata, archive names, OS `IMG_NAME` and flasher targets
  with that full version wherever those artifacts are being produced.
- Use worktree-local build, install, test settings and output directories.
- Record the full version, original branch, source commit, dirty state, target
  architecture and build timestamp beside the artifact. Dirty builds also need
  an archived source diff or equivalent source snapshot for reproducibility.
- Reconfigure and rebuild; check the actual binary's `--version` and Settings
  version display, then compare with the package/image metadata before delivery.
- Never relabel, copy or repackage an older binary as a newer product version.
  If building or verification is unavailable, report it and leave delivery pending.

## Active-release history

- 2026-09-09: User designated `origin/codex/v0.0.7` as the latest active unreleased
  base. Remote tip verified as `559edc3def`; older `0.0.6` checkouts are not bases
  for new feature work.
