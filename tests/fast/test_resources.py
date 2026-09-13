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
        root = ET.parse(SKIN / "waveforms.xml")
        waveforms = root.find(".//WidgetStack[@currentpage='[BiteDJ],training_mode']")
        normal = waveforms.find("./Children/WidgetGroup")
        phases = waveforms.findall("./Children/WidgetGroup/Children/TrainingPhase")
        training_layouts = waveforms.findall("./Children/WidgetGroup")[1:]
        panel = ET.parse(SKIN / "effects.xml").find(".//WidgetGroup")
        self.assertEqual("Waveforms", waveforms.findtext("ObjectName"))
        self.assertEqual("0me,0me", waveforms.findtext("Size"))
        self.assertEqual("WaveformsNormal", normal.findtext("ObjectName"))
        self.assertEqual("0me,0me", normal.findtext("Size"))
        self.assertEqual(2, len(phases))
        self.assertEqual(
            ["TrainingPhaseLine", "TrainingPhaseBoxes"],
            [phase.findtext("ObjectName") for phase in phases],
        )
        self.assertTrue(all(phase.findtext("Size") == "0me,0me" for phase in phases))
        self.assertEqual(
            ["TrainingLineLayout", "TrainingBoxesLayout"],
            [layout.findtext("ObjectName") for layout in training_layouts],
        )
        self.assertTrue(
            all(
                layout.findtext("./Children/WidgetGroup/Size") == "126f,0me"
                for layout in training_layouts
            )
        )
        self.assertEqual("BeatFX_Container", panel.findtext("ObjectName"))
        self.assertEqual("204f,0me", panel.findtext("Size"))

    def test_deck_transport_has_safety_gap_and_overview_follows_time_mode(self):
        waveform = ET.parse(SKIN / "waveform.xml")
        gap = waveform.find(".//WidgetGroup[ObjectName='WaveformTransportSafetyGap']")
        self.assertIsNotNone(gap)
        self.assertEqual("0me,12f", gap.findtext("Size"))

        deck = ET.parse(SKIN / "deck.xml")
        overview = deck.find(".//Overview")
        mode_connections = [
            connection
            for connection in overview.findall("Connection")
            if connection.findtext("BindProperty") == "timeDisplayMode"
        ]
        self.assertEqual(1, len(mode_connections))
        self.assertEqual(
            "[Skin],deck<Variable name=\"channel\" />_time_mode",
            ET.tostring(mode_connections[0].find("ConfigKey"), encoding="unicode")
            .replace("<ConfigKey>", "")
            .replace("</ConfigKey>", "")
            .strip(),
        )

    def test_training_mode_masks_every_deck_bpm_readout(self):
        deck = ET.parse(SKIN / "deck.xml")
        grid = ET.parse(SKIN / "templates/grid_deck.xml")
        self.assertFalse(deck.findall(".//NumberBpm"))
        self.assertFalse(grid.findall(".//NumberBpm"))
        self.assertEqual(2, len(deck.findall(".//TrainingBpm")))
        self.assertEqual(1, len(grid.findall(".//TrainingBpm")))
        self.assertTrue(
            all(
                "visual_bpm" in ET.tostring(widget, encoding="unicode")
                for widget in deck.findall(".//TrainingBpm")
            )
        )
        self.assertIn(
            "file_bpm",
            ET.tostring(grid.find(".//TrainingBpm"), encoding="unicode"),
        )

    def test_library_actions_stay_out_of_general_settings(self):
        root = ET.parse(SKIN / "settings.xml")
        general = root.find(".//WidgetGroup[@trigger='[SettingsTab],general']")
        library = root.find(".//WidgetGroup[@trigger='[SettingsTab],columns']")
        general_xml = ET.tostring(general, encoding="unicode")
        library_xml = ET.tostring(library, encoding="unicode")
        for key in (
            "[Library],grid_layout",
            "[Library],reset_played_tracks",
            "[Library],clear_cached_waveforms",
            "[Library],clear_cue_overrides",
            "[Library],clear_meta_overrides",
        ):
            self.assertNotIn(key, general_xml)
            self.assertIn(key, library_xml)
        self.assertNotIn("[BiteDJ],phase_meter", general_xml)
        training_values = {
            variable.text
            for template in general.iter("Template")
            for variable in template.findall("SetVariable")
            if variable.get("name") == "Value"
            and any(
                sibling.get("name") == "ConfigKey"
                and sibling.text == "[BiteDJ],training_mode"
                for sibling in template.findall("SetVariable")
            )
        }
        self.assertEqual({"0", "1", "2"}, training_values)

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
