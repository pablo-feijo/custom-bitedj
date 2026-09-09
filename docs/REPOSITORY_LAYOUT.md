# Repository layout

<!-- Modified for Custom Bite DJ on 2026-09-09: clarify fork identity and attribution. -->

Project scope: [Custom Bite DJ](../README.md), the independent BiteDJ fork.

Keep the root for project entry points and configuration: README, changelog,
licenses/notices, agent instructions, CMakeLists.txt, Doxyfile and tool settings.
BiteDJ helper scripts and Docker recipes live in the directories below.

| Location | Contents |
| --- | --- |
| `src/` | Application C++ and native tests in `src/test/` |
| `res/` | Skins, controllers and other shipped resources |
| `lib/`, `cmake/` | Bundled dependencies and CMake support |
| `scripts/build/` | ARM64 Docker build and Pi image generation |
| `scripts/deploy/` | SSH deployment and SD-card flashing |
| `scripts/test/` | GUI launcher, ownership/settings helper, capture and smoke scripts, synthetic music generator, audio server and noVNC audio snippet |
| `scripts/legacy/` | Retained Ubuntu library extraction helpers; not part of the current build |
| `docker/` | Builder, GUI test and legacy test Dockerfiles |
| `tests/` | Fast contracts, desktop E2E, integration fixtures and feature-specific test procedures; see [test strategy](TESTING.md) |
| `tools/` | Existing upstream development, packaging and CI utilities |
| `packaging/`, `.github/` | Distribution packaging and CI workflows |
| `docs/` | Guides, plans, checklists and curated documentation assets |
| `mixxx-pi-gen/` | Pinned image-generator submodule; follow its own agent guide |
| `build-linux/`, `dist-linux/` | Ignored, worktree-local build and installed output |
| `test-config/`, `test-music/`, `test-results/` | Ignored, worktree-local runtime settings, generated music and test artifacts |

## Common commands

From the repository root:

```sh
./scripts/build/docker-build.sh --platform linux/arm64
./scripts/test/run-gui-test.sh
source ./scripts/test/gui-test-settings.sh
verify_test_instance_owner
```

Image and hardware operations retain their existing behavior:

```sh
./scripts/build/generate-pi-image.sh
./scripts/deploy/deploy-ssh.sh <raspberry_pi_ip>
./scripts/deploy/flash-sdcard.sh
```

Scripts resolve the worktree root from their own location, so an absolute script
path can also be invoked from another working directory. Build/install paths,
container ownership labels, settings and results remain rooted in that worktree.

Docker recipes require the repository root as their build context:

```sh
docker build --platform linux/arm64 -f docker/build.Dockerfile -t bitedj-builder-linux-arm64 .
docker build -f docker/gui-test.Dockerfile -t bitedj-gui-test .
```

`docker/legacy-test.Dockerfile` retains the older 1280×800 experimental setup;
use the GUI launcher for current 1024×600 verification. The two legacy library
helpers extract Ubuntu ARM64 runtime libraries into ignored
`test-results/legacy-libs/`; they are not dependencies of the current build.

## Rules for additions and moves

- Place new files in the matching directory above. Extend an existing helper or
  create a narrowly named subdirectory before adding another root-level file.
- Add a root file only when a tool requires discovery there or it is a deliberate
  project entry point. Explain the reason in the change description.
- Keep reusable scripts and fixture definitions in Git; write generated output
  to the ignored runtime directories. Never place scratch scripts, archives,
  screenshots, logs or reports at the root.
- When moving a file, search the entire tracked repository, including hidden CI
  files, and update shell/Python callers, Docker COPY and context paths, docs,
  agent guides, tests and config references. Check the pinned submodule for
  relevant callers without modifying unrelated submodule content.
- Resolve paths from the script location, quote paths, preserve executable bits,
  and verify commands from both the root and an unrelated working directory.
- Update this map when introducing a new category. Link to it from other guides
  instead of maintaining competing directory maps.

Use [Docker maintenance](DOCKER_MAINTENANCE.md) to keep build and container disk usage bounded.
