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

    def test_play_waveforms_expand_beside_fixed_control_panel(self):
        waveforms = ET.parse(SKIN / "waveforms.xml").find(".//WidgetGroup")
        panel = ET.parse(SKIN / "effects.xml").find(".//WidgetGroup")
        self.assertEqual("Waveforms", waveforms.findtext("ObjectName"))
        self.assertEqual("0me,0me", waveforms.findtext("Size"))
        self.assertEqual("BeatFX_Container", panel.findtext("ObjectName"))
        self.assertEqual("204f,0me", panel.findtext("Size"))

    def test_top_tabs_remain_select_only(self):
        button = ET.parse(SKIN / "tab.xml").find(".//PushButton")
        self.assertEqual("true", button.findtext("LeftClickIsPushButton"))
        connection = button.find("Connection")
        self.assertEqual("true", connection.findtext("EmitOnDownPress"))

    def test_skin_waveform_choices_keep_high_detail_opt_in(self):
        # Saved renderer IDs are a public skin/control contract. High-detail
        # legacy (7/12/16) and textured (22/23/24) types must not become the
        # choices exposed by the appliance skin.
        choices = {}
        for path in SKIN.rglob("*.xml"):
            for template in ET.parse(path).iter("Template"):
                variables = {v.get("name"): v.text for v in template.findall("SetVariable")}
                if variables.get("ConfigKey") == "[Waveform],waveform_type":
                    choices[variables["Text"]] = int(variables["Value"])
        self.assertEqual({"RGB": 17, "Filt": 19, "3 Band": 25}, choices)
