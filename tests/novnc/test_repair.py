"""Regression coverage for fresh and cached noVNC JavaScript patches."""
import importlib.util
from pathlib import Path
import unittest

root = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('repair', root / 'scripts/test/repair-novnc.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class RepairTest(unittest.TestCase):
    def test_fresh_and_cached_versions_are_idempotent(self):
        declaration = 'export let supportsWebCodecsH264Decode = false;'
        for source in (
            '/* older package */\nexport let dragThreshold = 10;',
            declaration + '\n/* current package */\nsupportsWebCodecsH264Decode = await _checkWebCodecsH264DecodeSupport();',
            declaration + r'\n' + declaration + '\n/* broken cached image */',
        ):
            with self.subTest(source=source):
                fixed = module.repair(source)
                self.assertEqual(fixed.count(declaration), 1)
                self.assertNotIn(declaration + r'\n', fixed)
                self.assertNotIn('= await _checkWebCodecsH264DecodeSupport()', fixed)
                self.assertEqual(module.repair(fixed), fixed)


if __name__ == '__main__':
    unittest.main()
