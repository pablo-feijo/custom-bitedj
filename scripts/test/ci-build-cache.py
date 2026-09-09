#!/usr/bin/env python3
"""Fingerprint compiled inputs and preserve CI binaries without relabeling them."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
ASSETS = ("res/skins/", "res/controllers/", "res/effects/", "res/keyboard/")
DOCS = {"README.md", "CHANGELOG.md", "AGENTS.md", "agents.md", ".codex/config.toml"}


def is_agent_documentation(path):
    # Native skill instructions/metadata do not affect compiled binaries.
    # Keep executable helpers and other skill assets in the fingerprint.
    return path.startswith(".agents/skills/") and (
        path.endswith(".md") or path.endswith("/agents/openai.yaml"))


def git(*args):
    return subprocess.check_output(["git", *args], cwd=ROOT)


def compiled_entries():
    entries = []
    embedded = set()
    for entry in git("ls-tree", "-rz", "HEAD").split(b"\0"):
        if not entry:
            continue
        metadata, raw_path = entry.split(b"\t", 1)
        path = raw_path.decode()
        entries.append((path, metadata.decode()))
        if path.endswith(".qrc"):
            tree = ET.fromstring(git("show", "HEAD:" + path))
            for node in tree.iter("file"):
                embedded.add(os.path.relpath(
                    os.path.normpath(ROOT / Path(path).parent / node.text), ROOT))
    return [(path, metadata) for path, metadata in entries
            if path in embedded or not (
                path.startswith(ASSETS) or path.startswith("docs/") or path in DOCS
                or is_agent_documentation(path))]


def fingerprint(environment):
    payload = {"schema": 1, "inputs": compiled_entries(),
               "environment": environment, "workspace": str(ROOT)}
    return hashlib.sha256(json.dumps(payload, sort_keys=True).encode()).hexdigest()


def snapshot(directory, key):
    directory.mkdir(parents=True, exist_ok=True)
    # Only executable outputs and shared libraries; no object/static-library cache.
    outputs = [ROOT / "build/mixxx-test"]
    outputs += [p for p in (ROOT / "build").rglob("*.so*") if p.is_file()]
    for source in outputs:
        dest = directory / source.relative_to(ROOT)
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, dest, follow_symlinks=False)
    shutil.copytree(ROOT / "dist-linux", directory / "dist-linux", dirs_exist_ok=True,
                    symlinks=True)
    version = subprocess.check_output([str(ROOT / "dist-linux/bin/mixxx"), "--version"],
                                      text=True, env={**os.environ, "QT_QPA_PLATFORM": "offscreen"})
    manifest = {"key": key, "source_commit": git("rev-parse", "HEAD").decode().strip(),
                "source_branch": os.environ.get("GITHUB_REF_NAME", ""),
                "binary_version": version.strip()}
    (directory / "provenance.json").write_text(json.dumps(manifest, indent=2) + "\n")


def restore(directory, key):
    manifest = json.loads((directory / "provenance.json").read_text())
    if manifest["key"] != key:
        raise RuntimeError("Cached binary input fingerprint differs")
    for name in ("build", "dist-linux"):
        shutil.copytree(directory / name, ROOT / name, dirs_exist_ok=True, symlinks=True)
    for executable in ("build/mixxx-test", "dist-linux/bin/mixxx"):
        if not os.access(ROOT / executable, os.X_OK):
            raise RuntimeError("Missing cached executable: " + executable)
    # Replace directories completely, so deleted/renamed assets cannot linger.
    for prefix in ASSETS:
        source = ROOT / prefix
        dest = ROOT / "dist-linux/share/mixxx" / source.name
        if dest.exists():
            shutil.rmtree(dest)
        shutil.copytree(source, dest, symlinks=True)
    print(json.dumps(manifest, indent=2))
    summary = os.environ.get("GITHUB_STEP_SUMMARY")
    if summary:
        with open(summary, "a") as out:
            out.write("### Reused CI binaries\n\nOriginal provenance (not a new release):\n\n```json\n"
                      + json.dumps(manifest, indent=2) + "\n```\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("key", "snapshot", "restore"))
    parser.add_argument("--environment", type=Path)
    parser.add_argument("--directory", type=Path, default=ROOT / "test-results/ci-native-cache")
    parser.add_argument("--key")
    args = parser.parse_args()
    if args.action == "key":
        print(fingerprint(args.environment.read_text()))
    elif args.action == "snapshot":
        snapshot(args.directory, args.key)
    else:
        restore(args.directory, args.key)


if __name__ == "__main__":
    main()
