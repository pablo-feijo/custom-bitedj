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
    if not re.search(r'export\s+(?:let|const|var)\s+supportsWebCodecsH264Decode\b', source):
        source = declaration + '\n' + source
    return source.replace(
        'supportsWebCodecsH264Decode = await _checkWebCodecsH264DecodeSupport();',
        'supportsWebCodecsH264Decode = false;')


if __name__ == '__main__':
    path = Path(sys.argv[1] if len(sys.argv) > 1 else '/usr/share/novnc/core/util/browser.js')
    original = path.read_text()
    fixed = repair(original)
    if fixed != original:
        path.write_text(fixed)
