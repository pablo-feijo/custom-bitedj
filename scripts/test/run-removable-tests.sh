#!/usr/bin/env bash
# Fixture mounts exist only inside a private Linux mount namespace.
set -euo pipefail
REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
if [[ "$(uname -s)" != Linux ]]; then
    echo "Removable-drive tests need Linux mount namespaces (use the builder container)." >&2
    exit 1
fi
NAMESPACE_ARGS=(--mount --propagation private)
if [[ "$EUID" != 0 ]]; then
    NAMESPACE_ARGS+=(--user --map-root-user)
fi
# The one-stick sampler case must not see the store fixture's extra USB drive.
for fixture in stores samplers; do
    unshare "${NAMESPACE_ARGS[@]}" bash -c '
        set -euo pipefail
        mount -t tmpfs -o size=128m tmpfs /mnt
        if [[ "$2" == stores ]]; then
            mkdir -p /mnt/usbtest
            mount -t tmpfs -o size=64m tmpfs /mnt/usbtest
        fi
        exec python3 "$1/scripts/test/run-tests.py" removable --fixture "$2" "${@:3}"
    ' bash "$REPO_DIR" "$fixture" "$@"
done
