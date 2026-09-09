"""Regression coverage for old GUI images and repeated launcher runs."""
import importlib.util
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location(
    "repair_novnc", ROOT / "scripts/test/repair-novnc.py"
)
repair_novnc = importlib.util.module_from_spec(spec)
spec.loader.exec_module(repair_novnc)


class NoVNCRepair(unittest.TestCase):
    def test_fresh_and_cached_images_remain_valid_after_repeated_repairs(self):
        declaration = "export let supportsWebCodecsH264Decode = false;"
        for prefix in ("", declaration + r"\n", declaration + "\n"):
            with self.subTest(prefix=prefix), tempfile.TemporaryDirectory() as temp:
                root = Path(temp)
                module = root / "core/util/browser.js"
                module.parent.mkdir(parents=True)
                module.write_text(prefix + "/* upstream */\n" + declaration + "\n")
                entry = root / "vnc.html"
                entry.write_text('<script type="module" src="./app/ui.js?v=old"></script>')
                repair_novnc.repair_tree(root)
                first = module.read_text(), entry.read_text()
                repair_novnc.repair_tree(root)
                self.assertEqual(first, (module.read_text(), entry.read_text()))
                self.assertEqual(1, module.read_text().count(declaration))
                self.assertNotIn("?v=old", entry.read_text())
                result = subprocess.run(
                    ["node", "--input-type=module", "--check"],
                    input=module.read_bytes(), capture_output=True, timeout=20,
                )
                self.assertEqual(0, result.returncode, result.stderr.decode())
