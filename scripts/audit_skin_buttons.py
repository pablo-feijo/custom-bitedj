#!/usr/bin/env python3
"""Audit reachable BiteDJ XML buttons; native widgets need a separate owner review."""
from pathlib import Path
import re
import xml.etree.ElementTree as ET

root = Path(__file__).resolve().parents[1] / 'res/skins/BiteDJ'
seen = set()
errors = []
buttons = 0

def walk(path):
    global buttons
    if path in seen:
        return
    seen.add(path)
    doc = ET.parse(path).getroot()
    for button in doc.iter('PushButton'):
        buttons += 1
        connections = button.findall('Connection')
        if not connections:
            errors.append(f'{path.relative_to(root)}: button has no connection')
        for connection in connections:
            key = connection.find('ConfigKey')
            if key is None:
                errors.append(f'{path.relative_to(root)}: connection has no key')
                continue
            # Keys supplied wholly by the calling template are reviewed at call sites.
            if key.find('Variable') is not None:
                continue
            text = ''.join(key.itertext()).strip()
            if not re.fullmatch(r'\[.+\],\S+', text):
                errors.append(f'{path.relative_to(root)}: malformed key {text!r}')
    for template in doc.iter('Template'):
        source = template.get('src', '')
        if source.startswith('skin:'):
            walk(root / source[5:])

walk(root / 'skin.xml')
print(f'{buttons} reachable button declarations in {len(seen)} XML files')
print('Connection presence/syntax only; this does not prove backend ownership or DSP.')
for error in errors:
    print(error)
raise SystemExit(bool(errors))
