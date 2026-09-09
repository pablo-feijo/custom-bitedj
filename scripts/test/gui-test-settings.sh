#!/usr/bin/env bash
# Shared by the launcher and GUI checks. Each worktree owns its own instance.
REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
if [[ -z "${BITEDJ_TEST_INSTANCE:-}" && -f "${REPO_DIR}/test-config/active-instance" ]]; then
    BITEDJ_TEST_INSTANCE="$(cat "${REPO_DIR}/test-config/active-instance")"
fi
if [[ -z "${BITEDJ_TEST_INSTANCE:-}" ]]; then
    WORKTREE_ID="$(printf '%s' "${REPO_DIR}" | cksum | awk '{print $1}')"
    BITEDJ_TEST_INSTANCE="bitedj-gui-${WORKTREE_ID}"
fi
if [[ ! "${BITEDJ_TEST_INSTANCE}" =~ ^[a-zA-Z0-9][a-zA-Z0-9_.-]*$ ]]; then
    echo "Invalid BITEDJ_TEST_INSTANCE: use letters, digits, dots, underscores or hyphens." >&2
    exit 1
fi
export BITEDJ_TEST_INSTANCE
CONTAINER_NAME="${BITEDJ_TEST_INSTANCE}"
RESULTS_DIR="${REPO_DIR}/test-results/${CONTAINER_NAME}"
CONFIG_DIR="${REPO_DIR}/test-config/${CONTAINER_NAME}"

verify_test_instance_owner() {
    local owner branch current_branch
    owner="$(docker inspect --format '{{ index .Config.Labels "us.bitedj.test.worktree" }}' "${CONTAINER_NAME}")"
    branch="$(docker inspect --format '{{ index .Config.Labels "us.bitedj.test.branch" }}' "${CONTAINER_NAME}")"
    current_branch="$(git -C "${REPO_DIR}" branch --show-current)"
    if [[ "${owner}" != "${REPO_DIR}" || "${branch}" != "${current_branch}" ]]; then
        echo "Refusing to operate on ${CONTAINER_NAME}: it belongs to another worktree/branch or has no ownership label." >&2
        return 1
    fi
}

test_host_port() {
    docker port "${CONTAINER_NAME}" "$1/tcp" | awk -F: 'NR == 1 {print $NF}'
}
