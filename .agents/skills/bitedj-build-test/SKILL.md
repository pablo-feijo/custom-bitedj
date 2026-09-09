---
name: bitedj-build-test
description: Build and test Custom Bite DJ, launch or diagnose an owned GUI preview, capture documentation images, or prepare Pi artifacts. Use before build, container, noVNC or hardware validation operations.
---

# Build and test Custom Bite DJ

Run repository scripts from the task worktree root. Choose the required
[test layer](../../../docs/TESTING.md#run-the-appropriate-layer) first.

- For a binary build, read [build and deployment entry points](references/tools.md#build-and-deployment-entry-points)
  and [branch versioning](../../../docs/BRANCH_VERSIONING.md). Check disk usage
  and use task-local outputs. Deliverables need rebuilt, verified versions.
- For GUI launch or manual container commands, read [instance ownership](references/tools.md#owned-gui-instance)
  before using the launcher or settings helper. Verify ownership and use actual
  assigned ports; preserve other tasks' containers.
- For screenshots or touch targeting, read [container tools](references/tools.md#container-tools)
  and [pixel scanning](references/tools.md#pixel-coordinate-scanning). Read the
  relevant [UI reference](../bitedj-ui/SKILL.md) before interacting.
- Before delivering a preview URL, pass [the noVNC gate](references/tools.md#novnc-delivery-gate),
  including a connected desktop in a real browser.
- For deployment or OS images, read [hardware reproducibility](references/tools.md#versioned-images-and-reproducible-hardware-fixes)
  and the [deployment guide](../../../docs/BUILD_AND_DEPLOY.md). Use existing
  task authorization; this skill does not authorize hardware deployment.

For docs/config/skill-only changes, check links, native skill discovery, config
parsing and the staged storage guard. Use compatible CI binaries when allowed
by the [cache protocol](../../../docs/TESTING.md#reuse-compiled-outputs-for-asset-changes);
rerun the applicable tests and retain original binary provenance.
