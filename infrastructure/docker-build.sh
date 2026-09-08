#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
IMAGE_NAME="bitedj-builder"
BUILD_DIR="${SCRIPT_DIR}/build-linux"
DIST_DIR="${SCRIPT_DIR}/dist-linux"

PLATFORM=""
CLEAN=false
RUN_TESTS=false
REBUILD_IMAGE=false

print_usage() {
    echo "Usage: $0 [options]"
    echo ""
    echo "Build BiteDJ for Linux inside a Docker container."
    echo ""
    echo "Options:"
    echo "  --platform <platform>  Target platform, e.g. linux/arm64 or linux/amd64 (default: host platform)"
    echo "  --clean                Remove existing build directory before building"
    echo "  --test                 Run unit tests (ctest) after compilation"
    echo "  --rebuild-image        Force rebuild of the Docker builder image"
    echo "  -h, --help             Show this help message"
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --platform)
            PLATFORM="$2"
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

PLATFORM_ARG=""
IMAGE_TAG_SUFFIX=""
if [[ -n "${PLATFORM}" ]]; then
    PLATFORM_ARG="--platform ${PLATFORM}"
    # Replace slashes for docker image tag suffix
    IMAGE_TAG_SUFFIX="-$(echo "${PLATFORM}" | tr '/' '-')"
fi
FULL_IMAGE_NAME="${IMAGE_NAME}${IMAGE_TAG_SUFFIX}"

if [[ "${CLEAN}" = true && -d "${BUILD_DIR}" ]]; then
    echo "==> Cleaning build directory: ${BUILD_DIR}"
    rm -rf "${BUILD_DIR}"
fi

mkdir -p "${BUILD_DIR}" "${DIST_DIR}"

# Build Docker image if not present or requested
if [[ "${REBUILD_IMAGE}" = true ]] || ! docker image inspect "${FULL_IMAGE_NAME}" >/dev/null 2>&1; then
    echo "==> Building Docker image: ${FULL_IMAGE_NAME} (${PLATFORM:-native})..."
    docker build ${PLATFORM_ARG} -t "${FULL_IMAGE_NAME}" -f "${SCRIPT_DIR}/Dockerfile" "${SCRIPT_DIR}"
fi

echo "==> Running build in container..."
docker run --rm \
    ${PLATFORM_ARG} \
    -v "${SCRIPT_DIR}:/src" \
    -v bitedj-ccache:/root/.cache/ccache \
    -w /src/build-linux \
    -e CCACHE_DIR=/root/.cache/ccache \
    "${FULL_IMAGE_NAME}" \
    bash -c "
        set -euo pipefail

        echo '==> Configuring CMake...'
        cmake -S /src -B . \
            -DCMAKE_BUILD_TYPE=RelWithDebInfo \
            -DBUILD_TESTING=$([[ "${RUN_TESTS}" = true ]] && echo 'ON' || echo 'OFF') \
            -DBUILD_BENCH=$([[ "${RUN_TESTS}" = true ]] && echo 'ON' || echo 'OFF') \
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

        echo '==> Compiling mixxx (bitedj)...'
        cmake --build . --parallel \$(nproc) --target mixxx

        echo '==> Installing to /src/dist-linux...'
        cmake --install .

        # Create bitedj symlink
        if [[ -f /src/dist-linux/bin/mixxx ]]; then
            ln -sf mixxx /src/dist-linux/bin/bitedj
        fi

        $([[ "${RUN_TESTS}" = true ]] && echo '
        echo \"==> Running tests...\"
        cmake --build . --parallel \$(nproc) --target mixxx-test
        export QT_QPA_PLATFORM=offscreen
        ctest --output-on-failure --timeout 45
        ' || true)

        echo '==> Build completed successfully!'
    "

echo ""
echo "============================================================"
echo " Build finished successfully!"
echo " Artifacts are available in: ${DIST_DIR}"
echo "   Binary:   ${DIST_DIR}/bin/mixxx (symlinked to ${DIST_DIR}/bin/bitedj)"
echo "   Assets:   ${DIST_DIR}/share/mixxx"
echo "============================================================"
