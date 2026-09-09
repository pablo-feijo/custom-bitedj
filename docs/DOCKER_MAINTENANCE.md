# Docker cleanup and disk recovery

Check space before large ARM64 builds or image generation:

```sh
df -h .
docker system df
```

Before each large build, aim for at least 10 GiB of free host space for object
files, static-library linking, test binaries and Docker logs. Below that budget,
clean obsolete task-owned outputs or perform the user-scoped Docker cleanup
before starting another build. Check usage again after the build and report
unusual growth. Keep only useful worktree-local test runs; do not accumulate
unbounded logs, raw captures or obsolete images. A link step can temporarily need a second copy of a
large archive. Build outputs remain isolated per worktree.

## Older branch previews and running containers

At build start and post-merge cleanup, inventory `docker ps -a`, container
ownership labels and bind mounts, `docker system df`, and host free space.
Age is a candidate signal, not sufficient proof that a container is obsolete.
Check whether its task is still active or its preview is still the user's
current review environment. A completed task's older preview can be retired
once it is superseded by the validated integration preview.

When the user authorizes cleanup of older branch containers, that scope includes
stopping the audited obsolete running previews on other branches. Proceed with
that authorized cleanup without another confirmation. Keep the current SemVer
preview, active feature previews and ongoing build/test containers. Build
containers may have no GUI ownership labels: inspect their `/src` mount and
processes before deciding. Do not use a global stop or infer inactivity merely
from a task being idle.

1. Record candidate IDs, names, branch/worktree labels, image IDs, writable-layer
   sizes and mounts in ignored `test-results/`. Review `docker diff` for unique
   files before removal. Settings/media/source on bind mounts remain on the host;
   disposable `/tmp` test recordings and screenshots leave with the container.
   Preserve unique uncommitted fixes or user data rather than archiving every
   reproducible cache or capture.
2. Recheck exact IDs and ownership immediately before acting. Compare mount sets
   by source/destination, not their returned array order. Stop and remove only
   the reviewed IDs:

   ```sh
   docker stop --timeout 5 "$reviewed_container_id"
   docker rm "$reviewed_container_id"
   ```

3. Leave named compiler caches and shared images needed by retained containers
   or active builds. A request to remove old runs does not require a global
   prune. Use the full-cleanup procedure below only for that broader scope.
4. Recheck retained instances, `docker system df` and `df -h .`. Report both the
   removed writable-layer bytes and the observed host free space. Container
   removal does not delete host bind-mounted builds, and Docker's VM disk may
   not immediately return the same number of bytes to the host. Concurrent
   builds can also change free space during cleanup.

Check large worktree-local `build-linux/`, `dist-linux/` and `test-results/`
separately when more disk recovery is needed. Retire known task-generated
objects, caches and temporary recordings under the existing
[worktree cleanup procedure](AGENTS.md#branch-cleanup-after-integration);
container cleanup alone does not authorize discarding unique source, settings,
release artifacts or another task's active outputs. Avoid starting duplicate
builds and keep monitoring free space during long compilation and linking.

## Full cleanup

When the user requests a full Docker cleanup/prune, use the full command rather
than stopping at dangling-image cleanup:

```sh
docker system prune --all --volumes --force
docker system df
df -h .
```

This removes stopped containers, unused networks, unused images, build cache
and unused anonymous volumes. Running containers and their attached images and
volumes remain. Named volumes are not removed by this command; inspect them
and obtain explicit scope before deleting persisted named-volume data. Never
stop running containers simply to make them eligible for pruning.

A full prune is global: stopped GUI instances from other worktrees are removed,
and their unused GUI images may need rebuilding. Worktree-local `test-config/`
settings and source files are bind-mounted host data and remain. Recreate a
needed GUI environment with that worktree's `scripts/test/run-gui-test.sh`;
read its newly assigned endpoints rather than assuming old ports. The GUI
recipe defaults to the ARM64 builder image produced by the build script; set
`BITEDJ_BUILDER_IMAGE` only when intentionally using another compatible builder. Verify
ownership before subsequent manual operations.

Use the explicit full-cleanup request as the exception to routine task isolation.
Do not silently run global prune on every build. For routine maintenance, keep
cleanup scoped to the current task and avoid deleting another task's container
or build outputs. Record Docker's reclaimed-space total in the task result.

## If Docker stops after disk exhaustion

1. Read the build error and check host free space. Source/config files are not
   cleanup targets. Recover space from the current task's reproducible outputs
   or the user-authorized Docker cleanup.
2. Check `docker desktop status` and Docker Desktop's error. If the engine has
   already stopped unexpectedly, `docker desktop restart` can recover it after
   space is available. Do not restart a healthy engine and interrupt other tasks.
3. Resume the worktree's build. Retain compiled objects when possible. Stripping
   debug information from this task's generated Linux objects is an optional
   space-saving recovery, performed inside the matching builder image; it does
   not replace rebuilding source changes or running tests.
4. Run the relevant native checks and recreate the owned GUI instance. A
   completed compilation alone does not prove the final link or GUI succeeded.

For focused native tests, the build helper accepts a quoted CTest regex:

```sh
./scripts/build/docker-build.sh --platform linux/arm64 --test-filter 'EffectSlotTest\.'
```

Logs and cleanup reports belong under ignored `test-results/`, never the root.
