"""Run with python3 -m unittest discover -s tests/effects."""
import importlib.util
from pathlib import Path
import unittest
import xml.etree.ElementTree as ET
ROOT = Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('beatfx', ROOT/'scripts/build/generate-beatfx.py')
module=importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

class CatalogueTest(unittest.TestCase):
    def test_generated_assets_and_unique_audio_payloads(self):
        files=sorted((ROOT/'res/effects/rekordbox7').glob('*.xml'))
        self.assertEqual(len(files),25)
        payloads=set()
        for file,(name,effects,note) in zip(files,module.CATALOG):
            self.assertEqual(file.read_bytes(),module.render(name,effects,note),file.name)
            root=ET.parse(file).getroot()
            payload=ET.tostring(root.find('Effects'))
            self.assertNotIn(payload,payloads,name)
            payloads.add(payload)
            self.assertTrue(root.findtext('Description').startswith('Native approximation:'))
    def test_pingpong_is_stereo_and_roll_variants_differ(self):
        presets={name:effects for name,effects,_ in module.CATALOG}
        self.assertEqual(presets['PING PONG'][0][1]['pingpong_amount'],1)
        self.assertEqual(presets['ECHO'][0][1]['pingpong_amount'],0)
        self.assertNotEqual(presets['ROLL'],presets['REV ROLL'])
        self.assertNotEqual(presets['ROLL'],presets['SLIP ROLL'])

if __name__=='__main__': unittest.main()
