#!/usr/bin/env bash
# Compatibility entry point; coverage is documented in docs/TESTING.md.
set -euo pipefail
REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
echo "Legacy GUI script replaced by the isolated E2E smoke suite."
exec python3 "${REPO_DIR}/scripts/test/run-tests.py" e2e "$@"
