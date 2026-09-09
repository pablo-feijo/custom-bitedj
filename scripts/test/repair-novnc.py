#!/usr/bin/env python3
"""Repair legacy BiteDJ noVNC injection and disable the WebCodecs probe.

Safe to run on both fresh packages and cached GUI images. The package already
exports the capability in newer versions; never prepend a duplicate declaration.
"""
from pathlib import Path
import re
import sys


def repair(source):
    declaration = 'export let supportsWebCodecsH264Decode = false;'
    # Older Dockerfiles injected a literal backslash-n before the opening comment.
    source = source.removeprefix(declaration + r'\n')
    if source.startswith(declaration + '\n') and source.count(declaration) > 1:
        source = source.removeprefix(declaration + '\n')
    if not re.search(r'export\s+(?:let|const|var)\s+supportsWebCodecsH264Decode\b', source):
        source = declaration + '\n' + source
    return source.replace(
        'supportsWebCodecsH264Decode = await _checkWebCodecsH264DecodeSupport();',
        'supportsWebCodecsH264Decode = false;')


def repair_tree(root):
    browser = root / "core/util/browser.js"
    browser.write_text(repair(browser.read_text()))
    # Version the module graph so browsers cannot reuse the broken cached modules.
    for asset in root.rglob("*"):
        if asset.suffix in (".js", ".html"):
            text = asset.read_text()
            text = re.sub(r"([.]js)(?:[?]v=[^\"\x27\s]+)?(?=[\"\x27])", r"\1?v=bitedj-20260909", text)
            asset.write_text(text)


if __name__ == "__main__":
    if len(sys.argv) > 1:
        path = Path(sys.argv[1])
        path.write_text(repair(path.read_text()))
    else:
        repair_tree(Path("/usr/share/novnc"))
