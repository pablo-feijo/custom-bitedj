"""Cheap source-level contracts; behavioral coverage belongs in C++/E2E."""

from pathlib import Path
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
SKIN = ROOT / "res/skins/BiteDJ"


class SkinContracts(unittest.TestCase):
    def test_skin_and_effect_chains_are_well_formed(self):
        paths = list(SKIN.rglob("*.xml")) + list(
            (ROOT / "res/effects/chains").glob("*.xml")
        )
        self.assertTrue(paths)
        for path in paths:
            with self.subTest(file=path.relative_to(ROOT)):
                ET.parse(path)

    def test_static_template_references_exist(self):
        for path in SKIN.rglob("*.xml"):
            for template in ET.parse(path).iter("Template"):
                source = template.get("src", "")
                if source.startswith("skin:"):
                    with self.subTest(file=path.name, reference=source):
                        self.assertTrue(
                            (SKIN / source[5:].lstrip("/")).is_file()
                        )

    def test_overview_preserves_supported_pages(self):
        root = ET.parse(SKIN / "effects.xml")
        stack = root.find(".//WidgetStack[@currentpage='[FxPanel],current']")
        self.assertIsNotNone(stack)
        self.assertEqual(
            ["[FxPanel],fx", "[FxPanel],key", "[FxPanel],jump", "[FxPanel],grid"],
            [page.get("trigger") for page in stack.find("Children")],
        )

    def test_top_tabs_remain_select_only(self):
        button = ET.parse(SKIN / "tab.xml").find(".//PushButton")
        self.assertEqual("true", button.findtext("LeftClickIsPushButton"))
        connection = button.find("Connection")
        self.assertEqual("true", connection.findtext("EmitOnDownPress"))
