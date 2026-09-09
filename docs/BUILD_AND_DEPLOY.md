# Building and Deploying BiteDJ

<!-- Modified for Custom Bite DJ on 2026-09-09: clarify fork identity and attribution. -->

This guide describes [Custom Bite DJ](../README.md), an independent fork of
[Team Deckshark’s BiteDJ](https://github.com/TeamDeckshark/bitedj), based on Mixxx.

This guide outlines the workflows for building the BiteDJ binary and generating the Raspberry Pi OS image.

## Prerequisites
- macOS or Linux development machine
- Docker Desktop installed and running
- SSH access to your Raspberry Pi (for hot deployments)

## 1. Generating a Complete OS Image
If you need to flash a brand-new Raspberry Pi, you must bake a complete OS image containing Sway, Wayland, and all of BiteDJ's dependencies. Note: The image must be based on Debian 13 (Trixie) to provide the modern libraries required by the binary (e.g. GLIBC 2.38, Qt 6.8, libFLAC 14, FFmpeg 7).

Run the image generator script:
```bash
./scripts/build/generate-pi-image.sh
```
This script will:
1. Validate the existing ARM64 install against its source/version and content manifest; rebuild missing or stale artifacts via `scripts/build/docker-build.sh`.
2. Require the pinned `mixxx-pi-gen` checkout and an `IMG_NAME` ending in `-v<full-binary-version>`, then launch its Docker container.
3. Build a customized Debian Trixie OS from scratch (Stages 0-3).
4. Export a fully flashable `.zip` file into `mixxx-pi-gen/deploy/`.

**Flashing the SD Card:**
Once the `.zip` is generated, insert your SD card and run:
```bash
./scripts/deploy/flash-sdcard.sh
```
Follow the interactive prompts to safely write the image to your disk.

**Expanding the Filesystem (Important):**
Because the raw image is deliberately kept small (~6GB) to speed up flashing, it will not automatically fill large SD cards. If you encounter "No space left on device" errors over SSH, you must manually expand the root partition:
```bash
# Connect to the Pi and run:
sudo raspi-config nonint do_expand_rootfs
sudo resize2fs /dev/mmcblk0p2
```

## 2. Hot-Deploying the Binary via SSH
If you already have a working BiteDJ Raspberry Pi and just need to update the application code, you do NOT need to burn a new OS image. You can hot-swap the binary over SSH.

**Compile the binary locally:**
```bash
./scripts/build/docker-build.sh --platform linux/arm64
```
*This creates the executable inside `dist-linux/bin/`.*

**Deploy to the Pi:**
Use the included `scripts/deploy/deploy-ssh.sh` script or run:
```bash
scp dist-linux/bin/mixxx pi@<YOUR_PI_IP>:/tmp/bitedj
ssh pi@<YOUR_PI_IP> "sudo mv /tmp/bitedj /usr/bin/bitedj && sudo chmod +x /usr/bin/bitedj"

# Restart LightDM to cleanly reload the graphical session and prevent black screens:
ssh pi@<YOUR_PI_IP> "sudo systemctl restart lightdm"
```
LightDM will cleanly restart the Sway compositor and immediately launch the new BiteDJ binary without crashing into a failed state.

## Incremental local builds and artifact verification

Before building, set the branch-specific prerelease in `CMakeLists.txt` as
specified by [the version guide](BRANCH_VERSIONING.md). For example, the first
build on `codex/build-workflow-review` uses
`0.0.7-codex-build-workflow-review.1`. The helper rejects a suffix belonging to
another development branch. Increment the number for each new deliverable.

The Docker helper selects Ninja and sizes compiler parallelism to CPU count,
physical memory and cgroup memory limits, budgeting 3 GiB per worker. Override
with `--jobs 2` or `BITEDJ_BUILD_JOBS=2` when needed. It prints ccache statistics
and preserves the shared compiler cache across worktrees and `--clean` builds.

Builder recipes are checked through Docker's layer cache on every build.
Recipe/platform tags identify inputs, and the runtime configuration/rootfs content fingerprint, platform and
Ninja generator are recorded in `build-linux/.bitedj-builder`. Existing Makefile
builds, a different architecture, or a changed builder require one explicit
`--clean` migration. This removes only this worktree's build directory, retaining
its previous installed artifacts until a new build passes verification.
`--rebuild-image` refreshes the base and rebuilds dependency layers without cache.

Installation first goes into an ignored worktree-local staging directory.
After requested tests pass, the helper checks the ELF architecture, runs the
binary's `--version` inside its builder, and rejects sources changed during the
build. Only then does it replace the contents of `dist-linux`, removing stale
assets. `dist-linux/build-provenance.json` records the version, source commit,
branch, dirty state, source/content hashes, builder image ID and timestamp.
Dirty builds include `source-snapshot.tar.gz` for the recorded source inputs.
The fingerprint is deliberately conservative: tracked files and nonignored
untracked files, including documentation, participate; the image-generator
submodule is checked separately by the image wrapper.

A failed compilation, test or version verification leaves the previous install
intact. The helper serializes builds within each worktree using
`.bitedj-build-lock`; after an interrupted process that cannot run its cleanup
trap, verify no build is running before removing that worktree's stale lock.

Verify an existing install without compiling:

```sh
python3 scripts/build/build-support.py verify --platform linux/arm64
```

The image wrapper performs this check automatically and recompiles on failure.
It also requires `mixxx-pi-gen/config` to name the verified product version,
including the prerelease suffix. Initialize the pinned submodule with
`git submodule update --init mixxx-pi-gen` if it is absent; do not substitute an
unrelated image-generator checkout. GUI and hardware testing requirements in
[the agent build/test guide](../.agents/skills/bitedj-build-test/references/tools.md) still apply before delivery.

BuildKit may refresh an OCI index ID when only attestations change. The helper
compares rootfs layer digests and runtime configuration for cache compatibility,
while keeping the exact resolved image ID in binary provenance. A prior image-ID
marker is migrated only if its pinned image remains inspectable and equivalent.
