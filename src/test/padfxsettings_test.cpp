// Standalone integration test: real ConfigObject + real ControlObjects, no mock.
#include <QCoreApplication>
#include <QDomDocument>
#include <QFile>
#include <QLayout>
#include <QTest>
#include "widget/wbeatgridcontrols.h"
#include "skin/legacy/skincontext.h"
#include "widget/wwidgetgroup.h"
#include "widget/wcontrollerpaddisplay.h"
#include <array>
#include "test/mixxxtest.h"
#include <QTemporaryDir>
#include <iostream>
#include <limits>
#include <stdexcept>

#include "control/control.h"
#include "control/controlobject.h"
#include "preferences/padfxsettings.h"

namespace {
void require(bool value, const char* message) {
    if (!value) { throw std::runtime_error(message); }
}
double get(const QString& item) {
    return ControlObject::get(ConfigKey("[PadFX]", item));
}
void set(const QString& item, double value) {
    ControlObject::set(ConfigKey("[PadFX]", item), value);
    QCoreApplication::processEvents();
}
} // namespace

class PadFxSettingsTest : public MixxxTest {};
TEST_F(PadFxSettingsTest, ValidationPersistenceAndIsolation) {
    QTemporaryDir dir;
    require(dir.isValid(), "temporary settings directory");
    const auto path = dir.filePath("padfx.cfg");
    auto config = UserSettingsPointer(new UserSettings(path, dir.path(), dir.path()));
    config->setValue(ConfigKey("[PadFX_v1]", "d1_s4_effect"), 999);
    config->setValue(ConfigKey("[PadFX_v2]", "future_value"), 123);
    ControlDoublePrivate::setUserConfig(config);
    {
        PadFxSettings settings(config);
        require(get("version") == 1, "control contract");
        require(get("d1_s4_effect") == 4, "invalid saved assignment fallback");
        require(get("d4_s15_effect") == 15, "four deck defaults");
        set("d2_s3_effect", 9);
        set("d2_s3_reset", 1); set("d2_s3_reset", 0);
        require(get("d2_s3_effect") == 3, "system reset without editor");
        set("slot", 4);
        require(get("effect") == 4 && get("strength") == 4, "selection refresh");
        set("effect", 7); set("beat", 6); set("strength", 2); set("hold", 1);
        require(get("d1_s4_effect") == 7, "editor assignment write");
        require(config->getValue<int>(ConfigKey("[PadFX_v1]", "d1_s4_effect"), -1) == 7, "editor persisted");
        set("deck", 1);
        require(get("effect") == 4 && get("strength") == 4, "deck isolation");
        set("d1_s4_strength", 1); set("deck", 0);
        require(get("strength") == 1, "direct control update reflected in editor");
        for (double bad : {-1.0, 900.0, 0.5, std::numeric_limits<double>::quiet_NaN(),
                     std::numeric_limits<double>::infinity()}) {
            set("effect", bad); require(get("effect") == 7, "invalid editor rejected");
            set("d1_s4_effect", bad); require(get("d1_s4_effect") == 7, "invalid direct write rejected");
            set("slot", bad); require(get("slot") == 0, "invalid slot guarded");
            set("deck", bad); require(get("deck") == 0, "invalid deck guarded");
            set("slot", 4);
        }
        require(config->save(), "settings saved to disk");
    }
    config = UserSettingsPointer(new UserSettings(path, dir.path(), dir.path()));
    ControlDoublePrivate::setUserConfig(config);
    {
        PadFxSettings settings(config);
        set("slot", 4);
        require(get("effect") == 7 && get("beat") == 6 && get("strength") == 1 && get("hold") == 1,
                "disk round trip");
        set("reset_slot", 1); set("reset_slot", 0);
        require(get("effect") == 4 && get("beat") == 0 && get("strength") == 4 && get("hold") == 0,
                "selected reset");
        set("effect", 9); set("reset_slot", 1); set("reset_slot", 0);
        require(get("effect") == 4, "reset is reusable");
        require(config->getValue<int>(ConfigKey("[PadFX_v2]", "future_value"), -1) == 123,
                "unknown future namespace preserved");
    }
    std::cout << "Pad FX native settings PASS: real CO/editor synchronization, validation, four decks, disk roundtrip, repeatable reset, future namespace preservation\n";
}

TEST_F(PadFxSettingsTest, TouchCycleWithoutControllerAndControllerSelectionShareState) {
    PadFxSettings settings(config());
    for (int expected : {4, 2, 1, 3, 0, 4}) {
        set("d1_cycle", 1);
        require(get("d1_mode") == expected, "touch cycle order");
        require(get("d1_performance_visible") == (expected > 0 && expected < 4), "matching grid");
        set("d1_cycle", 0);
        require(get("d1_mode") == expected, "release must not advance");
        require(get("d2_mode") == 0, "other deck unchanged");
    }
    set("d1_mode", 1); // Same write made by controller mode selection.
    set("d1_cycle", 1); set("d1_cycle", 0);
    require(get("d1_mode") == 3, "touch continues from controller-selected mode");
    set("d1_mode", 0);
    require(get("d1_performance_visible") == 0, "controller hot cue restores cue grid");
}

TEST_F(PadFxSettingsTest, TouchPreviousIsInverseAndWrapsIndependentlyForBothDecks) {
    PadFxSettings settings(config());
    for (const auto* prefix : {"d1_", "d2_"}) {
        const QString key(prefix);
        for (int mode = 0; mode < 5; ++mode) {
            set(key + "mode", mode);
            set(key + "cycle", 1); set(key + "cycle", 0);
            set(key + "previous", 1); set(key + "previous", 0);
            require(get(key + "mode") == mode, "previous reverses next");
            require(get(key + "performance_visible") == (mode > 0 && mode < 4), "previous selects correct grid");
        }
        set(key + "mode", 0);
        set(key + "previous", 1); set(key + "previous", 0);
        require(get(key + "mode") == 3, "previous wraps to Beat Loop");
    }
}


TEST_F(PadFxSettingsTest, TouchBankTransitionsKeepDrawerHeightStable) {
    // Exercise the actual skin's layout with every intermediate visibility
    // combination. Separate control notifications can expose two banks or none
    // before the selected bank settles; neither may resize the drawer.
    QFile file(ConfigObject<ConfigValue>::computeResourcePath() +
            "skins/BiteDJ/templates/cue_deck_page.xml");
    ASSERT_TRUE(file.open(QIODevice::ReadOnly));
    QDomDocument xml;
    ASSERT_TRUE(xml.setContent(&file));
    const auto page = xml.documentElement().firstChildElement("WidgetGroup");
    auto bankLayout = page;
    const auto groups = page.elementsByTagName("WidgetGroup");
    for (int i = 0; i < groups.size(); ++i) {
        const auto group = groups.at(i).toElement();
        if (group.firstChildElement("ObjectName").text() == "CuePanel_Banks") {
            bankLayout = group;
            break;
        }
    }
    PadFxSettings settings(config());
    SkinContext context(config(), "test");
    WWidgetGroup banks;
    banks.setup(bankLayout, context);
    // WWidgetGroup::setup handles Layout; the skin parser applies fixed Size.
    const QString height = bankLayout.firstChildElement("Size").text().section(',', 1);
    if (height.endsWith('f')) {
        banks.setFixedHeight(height.chopped(1).toInt());
    }
    std::array<QWidget*, 3> pages{};
    for (int i = 0; i < 2; ++i) {
        pages[i] = new QWidget(&banks);
        pages[i]->setMinimumSize(480, 92); // Two 44px cue rows and their 4px gap.
        banks.layout()->addWidget(pages[i]);
    }
    auto* legend = new WControllerPadDisplay(&banks);
    QDomDocument legendXml;
    ASSERT_TRUE(legendXml.setContent(QStringLiteral(
            "<ControllerPadDisplay><Channel>1</Channel></ControllerPadDisplay>")));
    legend->setup(legendXml.documentElement(), context);
    pages[2] = legend;
    banks.layout()->addWidget(legend);
    banks.show();
    // Real word-wrapped legend labels matter: placeholder rectangles alone
    // cannot detect their height-for-width interaction with the stacked layout.
    for (int width : {480, 1024}) {
        banks.resize(width, 92);
        for (int mode : {0, 4, 2, 1, 3, 0}) {
            set("d1_mode", mode);
            for (int mask = 0; mask < 8; ++mask) {
                for (int bank = 0; bank < 3; ++bank) {
                    pages[bank]->setVisible(mask & (1 << bank));
                }
                banks.layout()->activate();
                QCoreApplication::processEvents();
                EXPECT_EQ(banks.height(), 92)
                        << "width " << width << " mode " << mode << " mask " << mask;
                if (mask & 4) {
                    EXPECT_EQ(legend->height(), 92);
                }
            }
        }
    }
}

TEST_F(PadFxSettingsTest, GridHoldRepeatAndCancellation) {
    ControlObject earlier(ConfigKey("[Channel1]", "beats_translate_earlier"));
    ControlObject setGrid(ConfigKey("[Channel1]", "beats_translate_curpos"));
    ControlObject otherDeck(ConfigKey("[Channel2]", "beats_translate_earlier"));
    int presses = 0;
    QObject::connect(&earlier, &ControlObject::valueChanged, [&presses](double v) { if (v == 1) ++presses; });
    QDomDocument doc;
    ASSERT_TRUE(doc.setContent(QStringLiteral("<BeatGridControls><Channel>1</Channel></BeatGridControls>")));
    SkinContext context(config(), "test");
    WBeatGridControls widget;
    widget.setup(doc.documentElement(), context);
    widget.resize(204, 148);
    widget.show();
    QCoreApplication::processEvents();
    auto* button = widget.findChild<QPushButton*>("beats_translate_earlier");
    ASSERT_TRUE(button);
    EXPECT_EQ(350, button->autoRepeatDelay());
    EXPECT_EQ(80, button->autoRepeatInterval());
    QTest::mousePress(button, Qt::LeftButton);
    EXPECT_EQ(1, presses);
    QTest::qWait(250);
    EXPECT_EQ(1, presses);
    QTest::qWait(300);
    EXPECT_GE(presses, 3);
    EXPECT_DOUBLE_EQ(0, otherDeck.get());
    widget.hide();
    const int stopped = presses;
    QTest::qWait(200);
    EXPECT_EQ(stopped, presses);
    EXPECT_DOUBLE_EQ(0, earlier.get());
    widget.show();
    auto* setButton = widget.findChild<QPushButton*>("beats_translate_curpos");
    ASSERT_TRUE(setButton);
    EXPECT_FALSE(setButton->autoRepeat());
    int sets = 0;
    QObject::connect(&setGrid, &ControlObject::valueChanged, [&sets](double v) { if (v == 1) ++sets; });
    QTest::mousePress(setButton, Qt::LeftButton);
    QTest::qWait(550);
    QTest::mouseRelease(setButton, Qt::LeftButton);
    EXPECT_EQ(1, sets);
    EXPECT_DOUBLE_EQ(0, setGrid.get());
}
