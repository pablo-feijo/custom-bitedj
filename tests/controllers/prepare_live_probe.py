"""Create a test-only DDJ-400 preset from this checkout; output stays ignored."""
import argparse
from pathlib import Path
import shutil
import xml.etree.ElementTree as ET
p = argparse.ArgumentParser()
p.add_argument('destination', type=Path)
a = p.parse_args()
root = Path(__file__).resolve().parents[2]
a.destination.mkdir(parents=True, exist_ok=True)
for name in ('Pioneer-DDJ-400-script.js', 'piflex-padfx.js'):
    shutil.copy2(root / 'res/controllers' / name, a.destination / name)
shutil.copy2(Path(__file__).with_name('live_probe.js'), a.destination / 'live_probe.js')
tree = ET.parse(root / 'res/controllers/Pioneer-DDJ-400.midi.xml')
controller = tree.find('controller')
ET.SubElement(controller.find('scriptfiles'), 'file', filename='live_probe.js', functionprefix='BiteDJProbe')
control = ET.SubElement(controller.find('controls'), 'control')
for key, value in {'group': '[Master]', 'key': 'BiteDJProbe.command', 'status': '0xBF', 'midino': '0x7E'}.items():
    ET.SubElement(control, key).text = value
ET.SubElement(ET.SubElement(control, 'options'), 'Script-Binding')
tree.write(a.destination / 'Pioneer-DDJ-400.midi.xml', encoding='utf-8', xml_declaration=True)
