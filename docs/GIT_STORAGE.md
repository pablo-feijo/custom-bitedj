# Git storage and build retention

Keep source, generators and small curated assets in Git. New or changed ordinary
Git blobs must be at most **1 MiB**. Reduce larger assets or keep them in external
artifact storage. Generated builds, OS images, archives, test recordings, logs
and caches belong in ignored runtime directories, never Git. Optimize curated
screenshots before committing them.

Run `python3 scripts/test/check-git-storage.py` after staging and before committing.
CI checks the complete index, including staged content later changed on disk.
The parent repository exempts four inherited audio fixtures by exact path and
blob ID in `scripts/test/git-size-exceptions.json`; do not add new exemptions.
pi-gen has no oversized fixture exemptions.

Git LFS is deliberately **not configured** for either repository. GitHub rejects
new LFS uploads to these fork networks; this restriction does not apply to all
public repositories. See [GitHub’s fork rules](https://docs.github.com/en/repositories/working-with-files/managing-large-files/collaboration-with-git-large-file-storage).
Do not add LFS attributes, `.lfsconfig`, hooks or CI checkout settings here.

History cleanup requires explicit authorization: audit paths/refs, retain
recovery refs, preserve upstream credits, and remap dependent submodule gitlinks.
Publish submodule commits first, then use explicit expected-value force-with-lease
for changed refs. Old objects remain while a branch, tag, recovery ref or reflog
retains them; history edits do not reclaim ignored build directories.

## Reclaim past build space on every build task

Before a large build and after successful validation, inventory host usage,
worktree outputs and Docker bind mounts. Keep the current validated build/preview,
active builds, unique settings/media, and explicitly requested release artifacts.
Remove superseded reproducible build/install directories, temporary staging,
raw test captures and obsolete image/archive copies once ownership and inactivity
are established. Do not retain a second raw OS image when its required validated
archive suffices. Do not delete the last useful build before its replacement passes.

Record exact removed paths and reclaimed bytes in ignored `test-results/` (pi-gen
may use its ignored `work/`). An old timestamp alone does not prove inactivity.
Keep brief provenance and unique recovery data; do not archive reproducible caches
under new backup folders. Preserve named compiler caches and healthy running
containers. Use scoped cleanup by default; global Docker pruning requires explicit
scope. Aim for at least 10 GiB free before heavy builds. Report retained large
folders with a reason and next cleanup action.

## Commit identity

Use the configured project identity and verify it with `git var GIT_AUTHOR_IDENT`
and `git var GIT_COMMITTER_IDENT` before committing. For this fork the canonical
identity is `Pablo Feijo <devpablofeijo@gmail.com>`. Set repository-local `user.name`,
`user.email` and `user.useConfigOnly=true`; do not rely on machine-derived emails.
Keep upstream authors and coauthor trailers accurate. A `.mailmap` fixes display
only; actual historical corrections require an authorized history rewrite.

## Documentation placement

`docs/` contains reusable human and agent guides. Local roadmaps, task plans
and execution logs live in ignored `/tasks/`, never Git. Do not link
published guides to local-only files. Use short screenshot captions and link
to canonical control mappings/provenance.

During cleanup, remove completed execution plans and activity logs from `tasks/`
after moving durable facts into the relevant guide. Keep active plans and
unresolved backlog items; consolidate outstanding work instead of archiving
finished task folders indefinitely. Never commit `tasks/` or its backups.
