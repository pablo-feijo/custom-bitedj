# Build, test and GUI tools

[Codex setup](../../../../docs/CODEX.md). Read this guide when its task trigger applies.

## Build and deployment entry points

If the user asks you to compile or test the application, use the scripts under `scripts/`, as listed in [the repository layout](../../../../docs/REPOSITORY_LAYOUT.md):

- **Compiling for the Pi**: Run `./scripts/build/docker-build.sh --platform linux/arm64`. This uses a custom Docker container to cross-compile the binary into `dist-linux/`. Do not try to compile natively on a Mac or Windows machine using standard `CMake` unless you are explicitly building a local debug version.
- **Local GUI & Audio Testing**: Before deploying changes or building an OS image, verify the 1024×600 GUI and affected audio paths in the owned instance using [GUI_TESTING.md](../../../../docs/GUI_TESTING.md), [PAD_FX_TESTING.md](../../../../docs/PAD_FX_TESTING.md), and the [Rekordbox fixture procedure](../../../../tests/rekordbox/README.md) as applicable. Start interactive testing with `./scripts/test/run-gui-test.sh` and use its printed VNC/audio endpoints; never assume ports belong to this task.
- **Hot-Deploying**: Use `./scripts/deploy/deploy-ssh.sh` to push a newly compiled ARM64 binary to a live Raspberry Pi over the network.
- **Flashing the OS**: The complete Raspberry Pi OS is generated using `./scripts/build/generate-pi-image.sh`, which leverages the `mixxx-pi-gen` submodule.

## Choose validation before building

Read [test layers](../../../../docs/TESTING.md#run-the-appropriate-layer) and run the checks
appropriate to the changed inputs. Documentation-only work needs link/anchor,
diff and staged-storage checks; it does not require a binary or GUI launch.
For asset changes, follow [compatible compiled-output reuse](../../../../docs/TESTING.md#reuse-compiled-outputs-for-asset-changes);
always rerun applicable tests. Compiled input or toolchain changes invalidate
reuse. Deliverable builds still require a full branch version and provenance.

## Owned GUI instance

- Every worktree must have its own `build-linux/`, `dist-linux/`, test settings,
  results, and GUI container. Never share writable build/install/config directories
  between branches. A shared compiler cache and immutable Docker image are fine.
- Use `scripts/test/run-gui-test.sh` and `scripts/test/gui-test-settings.sh`: they derive a worktree-specific
  container name, assign free host ports by default, and refuse to replace a
  container labeled for another worktree/branch. Explicit names/ports may be set
  with `BITEDJ_TEST_INSTANCE`, `BITEDJ_TEST_WEB_PORT`, `BITEDJ_TEST_AUDIO_PORT`,
  and `BITEDJ_TEST_VNC_PORT`.
- Read the printed endpoints or `docker port`; **never assume port 6080 or the
  legacy `bitedj-gui-test-instance` belongs to this task**. Source
  `scripts/test/gui-test-settings.sh` and run `verify_test_instance_owner` before manual
  test operations. Use `$CONTAINER_NAME` in commands.
- For stopping Mixxx in an owned instance, use
  `docker exec "$CONTAINER_NAME" pkill -9 mixxx`. Never use `killall`, global
  container cleanup, or remove another task's test instance.
- Existing fixtures under the original `test-config/` are not automatically
  copied. New instances use `test-config/<instance>/`; explicitly choose any
  settings migration. Optional USB test media should be read-only and selected
  for that task; no developer-specific volume is mounted automatically.
- Build/test the feature branch and leave its VNC available when requested.
  Report its exact branch, instance, endpoints, validation results, and merge
  target. Merge or deploy to hardware only when requested.

- Launch and open branch previews using the [environment-variable recipe](../../../../docs/GUI_TESTING.md#environment-variables-for-launching-and-opening-a-branch-preview).
  Export task-specific `BITEDJ_TEST_*` values in that task's shell, source the
  settings helper, verify ownership, and discover actual ports before opening.
  `.env` files are not loaded automatically. Clear inherited overrides when
  changing tasks; never hard-code another instance's container name or URL.

## Container tools

- **Process Management**: Always use `pkill -9 mixxx` to terminate Mixxx in the verified `$CONTAINER_NAME` instance. Never call `killall` (not installed in container).
- **Restart Recipe**:
  ```bash
  docker exec "$CONTAINER_NAME" pkill -9 mixxx && sleep 1 && \
  docker exec -d "$CONTAINER_NAME" bash -c "DISPLAY=:99 QT_AUTO_SCREEN_SCALE_FACTOR=0 QT_ENABLE_HIGHDPI_SCALING=0 QT_SCALE_FACTOR=1.0 /dist-linux/bin/mixxx --settings-path /root/.mixxx /music/BiteDJ_Test_Groove_128BPM.wav /music/BiteDJ_Test_Techno_124BPM.wav --resourcePath /dist-linux/share/mixxx/ --full-screen --style Fusion"
  ```
- **Screenshot Capture**: Always use `DISPLAY=:99 scrot /tmp/screen.png` inside the container, then `docker cp` to host. Never use ImageMagick `import` (not installed).
- **Image Cropping**: Always use `ffmpeg -y -i <in.png> -vf "crop=<w>:<h>:<x>:<y>" <out.png>`. Do not assume Python `PIL` or OpenCV are installed.
- **Hot-Reloading Skins**: Remember `/dist-linux` is bind-mounted `:ro` from the host repository. Edit files in `res/skins/BiteDJ/` and copy them to `dist-linux/share/mixxx/skins/BiteDJ/` on the host to immediately update the container before restarting Mixxx.

## Pixel coordinate scanning

Convert screenshot to PPM (`ffmpeg -i screen.png screen.ppm`) and parse raw RGB bytes to find the exact bounding box of any colored element or text baseline before clicking.

## Balanced spacing

- When resolving cramped UI items (e.g., text hugging a header), do not guess small increments (+2px or +4px).
- Measure both available margins: `gap_above` and `gap_below`.
- The balanced target position is `(gap_above + gap_below) / 2`.
- Verify both the element's own QSS (`margin-top`, `padding-left`) and its container's layout alignment (`qproperty-layoutAlignment`).

## noVNC Delivery Gate

- Before delivering a test GUI URL, follow [the noVNC delivery gate](../../../../docs/GUI_TESTING.md#novnc-delivery-gate): pass fast and E2E JavaScript checks, then verify the connected desktop in a real browser. HTTP success alone does not establish a working client.
- Keep cached-image repair shared by automated and manual launch paths. Test fresh, malformed and repeatedly repaired sources; invalidate the full module graph when changing cached assets.

## Docker disk hygiene

Follow [Docker maintenance](../../../../docs/DOCKER_MAINTENANCE.md) before large builds and
when recovering from disk exhaustion. Inspect host and Docker disk usage before
and after heavy builds; aim for at least 10 GiB free before starting and remove
obsolete task-owned outputs when space falls below that budget. When
the user requests full cleanup, run `docker system prune --all --volumes --force`
and report reclaimed space; do not substitute dangling-only cleanup. This
explicit global request permits pruning stopped instances across worktrees.
Keep running containers and persisted named-volume data outside that cleanup.
Recreate needed test instances through their owned-worktree launchers and read
the new ports. Do not prune globally on every build or restart a healthy engine.

## Versioned images and reproducible hardware fixes

Follow [branch versions](../../../../docs/BRANCH_VERSIONING.md) before any deliverable build.
The active application release is 0.0.8 on `codex/v0.0.8`. The image recipe includes the 0.0.8 Pi recording/storage fixes; the parent
`mixxx-pi-gen` gitlink selects the exact image source. Start new changes on an
isolated feature branch, publish the image commit, then update the parent
gitlink. Advance semver only within the user's integration authorization.
Keep `.gitmodules` URL/branch valid so a recursive clone resolves the pinned
commit. The flasher derives IMG_NAME from the pinned config; use
`BITEDJ_IMAGE_DATE=YYYY-MM-DD` to select a build from a different day.
Do not copy UI resources into pi-gen: it consumes the matching parent ARM64
`dist-linux` build. Its sample mixxx.cfg is not installed on first boot.

## Prevent configuration drift

When resolving bugs on live hardware or applying hot-patches over SSH (e.g., editing `~/.config/sway/config` or modifying `gsettings` on the Pi), you **must immediately backport those changes to the local repository.**
- Never leave a live Pi in a state that cannot be exactly reproduced by `./scripts/build/generate-pi-image.sh`.
- If you fix a system issue, commit the corresponding changes to the `mixxx-pi-gen` submodule (e.g., injecting the fix into `i3.conf` or `01-run.sh`) so the local build state remains the absolute source of truth.

## Reproducible Test Assets

Keep test generators, synthetic fixture definitions, reusable scripts and test
procedures in Git. Put generated exports, audio captures, screenshots, logs,
benchmark snapshots, caches and test reports in ignored `test-results/` (or
other ignored runtime directories). Do not commit test-run results. Record
instance ownership and regenerate assets instead of copying personal music.


## Pi playback and recording regression checks

For load/recording changes, follow [Pi regression coverage](../../../../docs/TESTING.md#pi-recording-and-load-regression-coverage).
Preserve image configuration for V3D selection before Sway starts, one automounter
(udiskie), and the USB BFQ/64-request rule. Verify these after reboot rather than
relying on a runtime-only fix. Check DDJ PCM and MIDI recovery separately.

Use varied large WAVs plus compressed files, DDJ Load-button routing and repeated
Browse/Play transitions while recording to the playback drive. Observe file growth,
then Stop Recording and wait for Saved before terminating or restarting the app.
Never use SIGTERM to finalize a test recording. Decode the whole file, verify
header/frame counts and correlate measured gaps with source silence and load times.
No logged underrun is not proof of continuous audio; internal master capture is
not an analog DDJ measurement. Test each storage/hub arrangement independently.
Preserve the user's recordings; identify disposable test takes before cleanup.
