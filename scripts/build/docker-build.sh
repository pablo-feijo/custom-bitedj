#!/usr/bin/env bash
set -euo pipefail

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
IMAGE_NAME="bitedj-builder"
BUILD_DIR="${REPO_DIR}/build-linux"
DIST_DIR="${REPO_DIR}/dist-linux"

PLATFORM=""
CLEAN=false
RUN_TESTS=false
TEST_FILTER="."
REBUILD_IMAGE=false
JOBS="${BITEDJ_BUILD_JOBS:-}"

print_usage() {
    echo "Usage: $0 [options]"
    echo ""
    echo "Build BiteDJ for Linux inside a Docker container."
    echo ""
    echo "Options:"
    echo "  --platform <platform>  Target platform, e.g. linux/arm64 or linux/amd64 (default: host platform)"
    echo "  --jobs <number>        Override memory-aware compiler worker count"
    echo "  --clean                Remove existing build directory before building"
    echo "  --test                 Run unit tests (ctest) after compilation"
    echo "  --test-filter <regex>  Build/run only matching native tests (implies --test)"
    echo "  --rebuild-image        Force rebuild of the Docker builder image"
    echo "  -h, --help             Show this help message"
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --platform)
            PLATFORM="${2:?--platform requires a value}"
            shift 2
            ;;
        --jobs)
            JOBS="${2:?--jobs requires a positive integer}"
            shift 2
            ;;
        --clean)
            CLEAN=true
            shift
            ;;
        --test)
            RUN_TESTS=true
            shift
            ;;
        --test-filter)
            RUN_TESTS=true
            TEST_FILTER="${2:?--test-filter requires a regex}"
            shift 2
            ;;
        --rebuild-image)
            REBUILD_IMAGE=true
            shift
            ;;
        -h|--help)
            print_usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1" >&2
            print_usage
            exit 1
            ;;
    esac
done

if [[ -n "$JOBS" && ! "$JOBS" =~ ^[1-9][0-9]*$ ]]; then
    echo "ERROR: --jobs must be a positive integer" >&2
    exit 1
fi
if [[ -z "$PLATFORM" ]]; then
    arch="$(docker info --format '{{.Architecture}}')"
    case "$arch" in
        arm64|aarch64) PLATFORM=linux/arm64 ;;
        amd64|x86_64) PLATFORM=linux/amd64 ;;
        *) echo "Unsupported Docker architecture: $arch" >&2; exit 1 ;;
    esac
fi
case "$PLATFORM" in
    linux/arm64|linux/amd64) ;;
    *) echo "Unsupported platform: $PLATFORM" >&2; exit 1 ;;
esac

# Include working-tree recipe content. Docker's layer cache makes this cheap.
RECIPE_KEY="$(python3 - "$REPO_DIR" "$PLATFORM" <<'PYKEY'
import hashlib, pathlib, sys
root = pathlib.Path(sys.argv[1])
h = hashlib.sha256(sys.argv[2].encode())
for name in ('docker/build.Dockerfile', 'tools/debian_buildenv.sh'):
    h.update((root / name).read_bytes())
print(h.hexdigest()[:20])
PYKEY
)"
FULL_IMAGE_NAME="${IMAGE_NAME}-${PLATFORM//\//-}:$RECIPE_KEY"
mkdir -p "$REPO_DIR/test-results"
if ! mkdir "$REPO_DIR/.bitedj-build-lock" 2>/dev/null; then
    echo "ERROR: This worktree has an active build lock (.bitedj-build-lock)." >&2
    exit 1
fi
STATE_FILE=""
STAGING_DIR=""
trap '[[ -z "$STATE_FILE" ]] || rm -f "$STATE_FILE"; [[ -z "$STAGING_DIR" ]] || rm -rf "$STAGING_DIR"; rmdir "$REPO_DIR/.bitedj-build-lock"' EXIT
STATE_FILE="$(mktemp "$REPO_DIR/test-results/build-state.XXXXXX")"
STAGING_DIR="$(mktemp -d "$REPO_DIR/dist-linux-staging.XXXXXX")"
python3 "$REPO_DIR/scripts/build/build-support.py" state > "$STATE_FILE"

if [[ "$REBUILD_IMAGE" = true ]]; then
    docker build --platform "$PLATFORM" --pull --no-cache -t "$FULL_IMAGE_NAME" \
        -f "$REPO_DIR/docker/build.Dockerfile" "$REPO_DIR"
else
    docker build --platform "$PLATFORM" -t "$FULL_IMAGE_NAME" \
        -f "$REPO_DIR/docker/build.Dockerfile" "$REPO_DIR"
fi
IMAGE_ID="$(docker image inspect --format '{{.Id}}' "$FULL_IMAGE_NAME")"
# Preserve the documented GUI builder alias. Existing containers retain their image.
docker tag "$IMAGE_ID" "${IMAGE_NAME}-${PLATFORM//\//-}:latest"

if [[ "$CLEAN" = true ]]; then rm -rf "$BUILD_DIR"; fi
BUILDER_KEY="$(python3 "$REPO_DIR/scripts/build/build-support.py" builder-key --image "$IMAGE_ID")"
IDENTITY="$PLATFORM $BUILDER_KEY Ninja"
# Migrate a marker written by the initial image-ID implementation only when the
# old pinned image is still available and has identical toolchain contents.
if [[ -f "$BUILD_DIR/.bitedj-builder" ]]; then
    read -r old_platform old_image old_generator < "$BUILD_DIR/.bitedj-builder"
    if [[ "$old_platform" = "$PLATFORM" && "$old_generator" = Ninja && "$old_image" = sha256:* ]]; then
        old_key="$(python3 "$REPO_DIR/scripts/build/build-support.py" builder-key --image "$old_image" 2>/dev/null || true)"
        if [[ "$old_key" = "$BUILDER_KEY" ]]; then
            printf '%s\n' "$IDENTITY" > "$BUILD_DIR/.bitedj-builder"
        fi
    fi
fi
if [[ -f "$BUILD_DIR/CMakeCache.txt" ]]; then
    if [[ ! -f "$BUILD_DIR/.bitedj-builder" ]] || [[ "$(cat "$BUILD_DIR/.bitedj-builder")" != "$IDENTITY" ]]; then
        echo "ERROR: Existing build uses a different or legacy toolchain/generator. Rerun with --clean (compiler cache is retained)." >&2
        exit 1
    fi
fi
mkdir -p "$BUILD_DIR"
printf '%s\n' "$IDENTITY" > "$BUILD_DIR/.bitedj-builder"
# Worktree .git files refer to metadata outside /src; mount that metadata read-only.
GIT_COMMON="$(git -C "$REPO_DIR" rev-parse --path-format=absolute --git-common-dir)"
docker run --rm -i --platform "$PLATFORM" \
    -v "$REPO_DIR:/src" -v "$GIT_COMMON:$GIT_COMMON:ro" \
    -v bitedj-ccache:/root/.cache/ccache \
    -w /src/build-linux \
    -e CCACHE_DIR=/root/.cache/ccache -e CCACHE_COMPILERCHECK=content \
    -e CCACHE_BASEDIR=/src \
    -e BITEDJ_BUILD_JOBS="$JOBS" -e BITEDJ_RUN_TESTS="$RUN_TESTS" \
    -e BITEDJ_CTEST_FILTER="$TEST_FILTER" \
    -e BITEDJ_STAGE="/src/$(basename "$STAGING_DIR")" \
    "$IMAGE_ID" bash -s <<'BUILD'
set -euo pipefail
workers="${BITEDJ_BUILD_JOBS:-$(python3 /src/scripts/build/build-support.py jobs)}"
echo "==> Using $workers compiler workers"
ccache --zero-stats
trap 'ccache --show-stats' EXIT
tests=OFF
if [[ "$BITEDJ_RUN_TESTS" = true ]]; then tests=ON; fi
cmake -G Ninja -S /src -B . \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DBUILD_TESTING="$tests" -DBUILD_BENCH=OFF \
    -DQT6=ON \
    -DBATTERY=ON \
    -DBROADCAST=ON \
    -DBULK=ON \
    -DFFMPEG=ON \
    -DHID=ON \
    -DKEYFINDER=ON \
    -DLILV=ON \
    -DLOCALECOMPARE=ON \
    -DMAD=ON \
    -DMODPLUG=ON \
    -DOPUS=ON \
    -DQTKEYCHAIN=ON \
    -DVINYLCONTROL=ON \
    -DWAVPACK=ON \
    -DDOWNLOAD_MANUAL=OFF \
    -DINSTALL_USER_UDEV_RULES=OFF \
    -DDEBUG_ASSERTIONS_FATAL=OFF \
    -DWARNINGS_FATAL=OFF \
    -DCMAKE_INSTALL_PREFIX=/src/dist-linux
cmake --build . --parallel "$workers" --target mixxx
if [[ "$BITEDJ_RUN_TESTS" = true ]]; then
    cmake --build . --parallel "$workers" --target mixxx-test
    QT_QPA_PLATFORM=offscreen ctest --output-on-failure --no-tests=error --timeout 45 -R "$BITEDJ_CTEST_FILTER"
fi
cmake --install . --prefix "$BITEDJ_STAGE"
ln -sf mixxx "$BITEDJ_STAGE/bin/bitedj"
BUILD

python3 "$REPO_DIR/scripts/build/build-support.py" seal --directory "$STAGING_DIR" \
    --platform "$PLATFORM" --image "$IMAGE_ID" --state "$STATE_FILE"
# Only publish a complete, verified install; stale assets cannot survive reinstall.
# Keep the directory itself stable for existing task-owned bind mounts.
mkdir -p "$DIST_DIR"
python3 - "$STAGING_DIR" "$DIST_DIR" <<'PUBLISH'
import pathlib, sys, tempfile
source, dest = map(pathlib.Path, sys.argv[1:])
# Same-filesystem renames avoid copying a second full install. Preserve the
# existing directory inode for bind mounts and roll back failed operations.
with tempfile.TemporaryDirectory(prefix='dist-linux-staging.rollback-', dir=dest.parent) as temp:
    backup = pathlib.Path(temp)
    installed = []
    try:
        for path in dest.iterdir():
            path.rename(backup / path.name)
        for path in source.iterdir():
            target = dest / path.name
            path.rename(target)
            installed.append(target)
    except BaseException:
        for path in installed:
            path.rename(source / path.name)
        for path in backup.iterdir():
            path.rename(dest / path.name)
        raise

PUBLISH
echo "==> Verified artifacts: $DIST_DIR (build-provenance.json)"
