#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QFile>
#include <QApplication>
#include <QDialog>
#include <QPushButton>
#include <QTimer>
#include <QSet>
#include "widget/weffectchainpresetselector.h"
#include "widget/wbeatperiodpicker.h"
#include "skin/legacy/skincontext.h"
#include <cmath>
#include <vector>

#include "control/controlobject.h"
#include "effects/backends/builtin/filtereffect.h"
#include "effects/effectchain.h"
#include "effects/chains/standardeffectchain.h"
#include "effects/effectparameterslotbase.h"
#include "effects/effectslot.h"
#include "effects/effectsmanager.h"
#include "effects/presets/effectchainpreset.h"
#include "effects/presets/effectpreset.h"
#include "engine/effects/engineeffectsmanager.h"
#include "engine/effects/groupfeaturestate.h"
#include "test/mixxxtest.h"

class EffectSlotTest : public MixxxTest {};

// Adapted from xsploit/bitedj's 4c1dfec590 control-to-audio regression.
// Exercise our standard rack rather than importing the fork's Pad FX lanes.
TEST_F(EffectSlotTest, ProgrammaticEnableAndDisableReachAudio) {
    auto factory = std::make_shared<ChannelHandleFactory>();
    EffectsManager manager(config(), factory);
    ChannelHandleAndGroup output(factory->getOrCreateHandle("[MasterOutput]"), "[MasterOutput]");
    ChannelHandleAndGroup deck(factory->getOrCreateHandle("[Channel1]"), "[Channel1]");
    manager.registerOutputChannel(output);
    manager.registerInputChannel(output);
    manager.registerInputChannel(deck);
    manager.setup();
    auto chain = manager.getStandardEffectChain(0);
    ASSERT_TRUE(chain);
    auto slot = chain->getEffectSlot(0);
    ASSERT_TRUE(slot);
    const auto manifest = manager.getBackendManager()->getManifest(
            FilterEffect::getId(), EffectBackendType::BuiltIn);
    ASSERT_TRUE(manifest);
    slot->loadEffectWithDefaults(manifest);
    ASSERT_TRUE(slot->isLoaded());
    ControlObject::set(ConfigKey(StandardEffectChain::formatEffectSlotGroup(0, 0),
                               "parameter1_value"), 500.0);
    ControlObject::set(ConfigKey(chain->group(), "group_[Channel1]_enable"), 1.0);
    ControlObject::set(ConfigKey(chain->group(), "mix"), 1.0);
    // Start disabled through an external writer, then test the C++ setter alone.
    ControlObject::set(ConfigKey(StandardEffectChain::formatEffectSlotGroup(0, 0), "enabled"), 0.0);
    auto* engine = manager.getEngineEffectsManager();
    QCoreApplication::processEvents();
    constexpr int frames = 256;
    std::vector<CSAMPLE> buffer(frames * 2);
    long long frame = 0;
    GroupFeatureState features;
    auto energy = [&] {
        double result = 0;
        for (int block = 0; block < 50; ++block) {
            for (int i = 0; i < frames; ++i, ++frame) {
                buffer[2 * i] = buffer[2 * i + 1] = static_cast<CSAMPLE>(
                        0.25 * std::sin(2 * 3.141592653589793 * 8000 * frame / 48000));
            }
            engine->onCallbackStart();
            engine->processPostFaderInPlace(deck.handle(), output.handle(),
                    buffer.data(), buffer.size(), mixxx::audio::SampleRate(48000),
                    features, 1, 1, false);
            if (block > 10) {
                for (auto sample : buffer) {
                    result += sample * sample;
                }
            }
        }
        return result;
    };
    const auto bypass = energy();
    ASSERT_GT(bypass, 1.0);
    slot->setEnabled(true);
    EXPECT_LT(energy(), bypass * 0.05);
    slot->setEnabled(true); // unchanged state must remain enabled
    EXPECT_LT(energy(), bypass * 0.05);
    slot->setEnabled(false);
    EXPECT_NEAR(energy(), bypass, bypass * 0.01);
    slot->setEnabled(false);
    EXPECT_NEAR(energy(), bypass, bypass * 0.01);
    ControlObject::set(ConfigKey(StandardEffectChain::formatEffectSlotGroup(0, 0), "enabled"), 1.0);
    EXPECT_LT(energy(), bypass * 0.05);
}


TEST_F(EffectSlotTest, RekordboxCatalogueLoadsValidDistinctPresetsAndGroupedActivation) {
    auto factory = std::make_shared<ChannelHandleFactory>();
    EffectsManager manager(config(), factory);
    ChannelHandleAndGroup output(factory->getOrCreateHandle("[MasterOutput]"), "[MasterOutput]");
    ChannelHandleAndGroup deck(factory->getOrCreateHandle("[Channel1]"), "[Channel1]");
    manager.registerOutputChannel(output);
    manager.registerInputChannel(output);
    manager.registerInputChannel(deck);
    manager.setup();
    auto chain = manager.getStandardEffectChain(0);
    auto presets = manager.getChainPresetManager();
    const QStringList expected = QStringLiteral("DELAY|ECHO|SPIRAL|REVERB|REV DELAY|MT DELAY|PITCH ECHO|TRANS|PAN|FILTER|FLANGER|PHASER|SLIP ROLL|ROLL|REV ROLL|ROBOT|PITCH|ENIGMA JET|MOBIUS SAW|MOBIUS TRI|LOW CUT ECHO|PING PONG|HELIX|VINYL BRAKE|STRETCH").split('|');
    const auto sorted = presets->getPresetsSorted();
    ASSERT_GE(sorted.size(), expected.size() + 1);
    EXPECT_EQ(sorted[0]->name(), kNoEffectString);
    ControlObject::set(ConfigKey(chain->group(), "group_[Channel1]_enable"), 1.0);
    ControlObject::set(ConfigKey(chain->group(), "mix"), 1.0);
    auto* engine = manager.getEngineEffectsManager();
    constexpr int frames = 256;
    std::vector<CSAMPLE> buffer(frames * 2);
    GroupFeatureState features;
    for (int index = 0; index < expected.size(); ++index) {
        const auto preset = sorted[index + 1];
        SCOPED_TRACE(expected[index].toStdString());
        EXPECT_EQ(preset->displayName(), expected[index]);
        EXPECT_TRUE(preset->isReadOnly());
        EXPECT_FALSE(preset->description().isEmpty());
        ASSERT_LE(preset->effectPresets().size(), 3);
        for (const auto& effect : preset->effectPresets()) {
            auto manifest = manager.getBackendManager()->getManifest(effect);
            ASSERT_TRUE(manifest);
            for (const auto& parameter : effect->getParameterPresets()) {
                EffectManifestParameterPointer found;
                for (const auto& candidate : manifest->parameters()) {
                    if (candidate->id() == parameter.id()) {
                        found = candidate;
                        break;
                    }
                }
                ASSERT_TRUE(found) << parameter.id().toStdString();
                EXPECT_GE(parameter.value(), found->getMinimum());
                EXPECT_LE(parameter.value(), found->getMaximum());
            }
        }
        chain->loadChainPreset(preset);
        QCoreApplication::processEvents();
        for (int slot = 0; slot < preset->effectPresets().size(); ++slot) {
            EXPECT_TRUE(chain->getEffectSlot(slot)->isLoaded());
            EXPECT_EQ(ControlObject::get(ConfigKey(chain->getEffectSlot(slot)->getGroup(), "enabled")), 0);
        }
        ControlObject::set(ConfigKey(chain->getEffectSlot(0)->getGroup(), "enabled"), 1);
        QCoreApplication::processEvents();
        for (int slot = 0; slot < preset->effectPresets().size(); ++slot) {
            EXPECT_EQ(ControlObject::get(ConfigKey(chain->getEffectSlot(slot)->getGroup(), "enabled")), 1);
        }
        double energy = 0;
        double rightEnergy = 0;
        const bool pingPong = expected[index] == QStringLiteral("PING PONG");
        for (int block = 0; block < 400; ++block) {
            for (int i = 0; i < frames; ++i) {
                auto sample = static_cast<CSAMPLE>(0.1 * std::sin(
                        2 * 3.141592653589793 * 440 * (block * frames + i) / 48000));
                buffer[2 * i] = sample;
                buffer[2 * i + 1] = pingPong ? 0.0f : sample * 0.7f;
            }
            engine->onCallbackStart();
            engine->processPostFaderInPlace(deck.handle(), output.handle(),
                    buffer.data(), buffer.size(), mixxx::audio::SampleRate(48000),
                    features, 1, 1, false);
            for (int i = 1; i < frames * 2; i += 2) {
                rightEnergy += buffer[i] * buffer[i];
            }
            for (auto sample : buffer) {
                ASSERT_TRUE(std::isfinite(sample));
                EXPECT_LT(std::abs(sample), 4.0);
                energy += sample * sample;
            }
        }
        EXPECT_GT(energy, 0.01);
        if (pingPong) {
            EXPECT_GT(rightEnergy, 0.01) << "Left-only input must reach the right echo channel";
        }
        ControlObject::set(ConfigKey(chain->getEffectSlot(0)->getGroup(), "enabled"), 0);
        QCoreApplication::processEvents();
        for (int slot = 0; slot < preset->effectPresets().size(); ++slot) {
            EXPECT_EQ(ControlObject::get(ConfigKey(chain->getEffectSlot(slot)->getGroup(), "enabled")), 0);
        }
    }
    chain->loadChainPreset(sorted[1]);
    ControlObject::set(ConfigKey(chain->group(), "chain_selector"), 1);
    EXPECT_EQ(chain->presetName(), sorted[2]->name());
    ControlObject::set(ConfigKey(chain->group(), "chain_selector"), -1);
    EXPECT_EQ(chain->presetName(), sorted[1]->name());
    ControlObject::set(ConfigKey(chain->group(), "chain_selector"), -1);
    EXPECT_EQ(chain->presetName(), kNoEffectString);
    ControlObject::set(ConfigKey(chain->group(), "chain_selector"), -1);
    EXPECT_EQ(chain->presetName(), sorted.last()->name());
}

TEST_F(EffectSlotTest, BeatPeriodAliasMatchesEchoAndTremoloAndClampsTruthfully) {
    auto factory = std::make_shared<ChannelHandleFactory>();
    EffectsManager manager(config(), factory);
    ChannelHandleAndGroup output(factory->getOrCreateHandle("[MasterOutput]"), "[MasterOutput]");
    ChannelHandleAndGroup deck(factory->getOrCreateHandle("[Channel1]"), "[Channel1]");
    manager.registerOutputChannel(output);
    manager.registerInputChannel(output);
    manager.registerInputChannel(deck);
    manager.setup();
    auto chain = manager.getStandardEffectChain(0);
    auto presets = manager.getChainPresetManager();
    for (const auto& name : {QStringLiteral("[RB7] ECHO"), QStringLiteral("[RB7] TRANS")}) {
        auto preset = presets->getPreset(name);
        ASSERT_TRUE(preset);
        chain->loadChainPreset(preset);
        const auto group = chain->getEffectSlot(0)->getGroup();
        int beatSlot = 0;
        for (int i = 1; i <= 16; ++i) {
            if (ControlObject::get(ConfigKey(group, QString("parameter%1_units").arg(i))) == 1) {
                beatSlot = i;
                break;
            }
        }
        ASSERT_GT(beatSlot, 0);
        const auto prefix = QString("parameter%1").arg(beatSlot);
        QDomDocument pickerDoc;
        ASSERT_TRUE(pickerDoc.setContent(QString("<BeatPeriodPicker><EffectGroup>%1</EffectGroup><Parameter>%2</Parameter></BeatPeriodPicker>").arg(group).arg(beatSlot)));
        SkinContext pickerContext(config(), "test");
        WBeatPeriodPicker picker;
        picker.setup(pickerDoc.documentElement(), pickerContext);
        auto* quarter = picker.findChild<QPushButton*>("BeatPeriod1");
        auto* four = picker.findChild<QPushButton*>("BeatPeriod5");
        ASSERT_TRUE(quarter);
        ASSERT_TRUE(four);
        EXPECT_EQ(name.endsWith("TRANS"), four->isEnabled());
        EXPECT_EQ(!name.endsWith("TRANS"), four->isHidden());
        quarter->click();
        EXPECT_DOUBLE_EQ(0.25, ControlObject::get(ConfigKey(group, prefix + "_beat_period")));
        EXPECT_TRUE(quarter->isChecked());

        EXPECT_LE(ControlObject::get(ConfigKey(group, prefix + "_beat_period_min")), 0.125);
        EXPECT_GE(ControlObject::get(ConfigKey(group, prefix + "_beat_period_max")),
                name.endsWith("TRANS") ? 4.0 : 2.0);
        ControlObject::set(ConfigKey(group, prefix + "_beat_period"), 0.125);
        EXPECT_DOUBLE_EQ(ControlObject::get(ConfigKey(group, prefix + "_value")),
                name.endsWith("TRANS") ? 8.0 : 0.0);
        EXPECT_DOUBLE_EQ(ControlObject::get(ConfigKey(group, prefix + "_beat_period")), 0.125);
        ControlObject::set(ConfigKey(group, prefix + "_beat_period"), 4.0);
        EXPECT_DOUBLE_EQ(ControlObject::get(ConfigKey(group, prefix + "_beat_period")),
                name.endsWith("TRANS") ? 4.0 : 2.0);
    }
}

TEST_F(EffectSlotTest, GlitchMinimumKeepsItsNativeEighthBeatValue) {
    auto factory = std::make_shared<ChannelHandleFactory>();
    EffectsManager manager(config(), factory);
    ChannelHandleAndGroup output(factory->getOrCreateHandle("[MasterOutput]"), "[MasterOutput]");
    ChannelHandleAndGroup deck(factory->getOrCreateHandle("[Channel1]"), "[Channel1]");
    manager.registerOutputChannel(output);
    manager.registerInputChannel(output);
    manager.registerInputChannel(deck);
    manager.setup();
    auto slot = manager.getStandardEffectChain(0)->getEffectSlot(0);
    const auto manifest = manager.getBackendManager()->getManifest(
            QStringLiteral("org.mixxx.effects.glitch"), EffectBackendType::BuiltIn);
    ASSERT_TRUE(manifest);
    slot->loadEffectWithDefaults(manifest);
    const auto group = slot->getGroup();
    bool found = false;
    for (int parameter = 1; parameter <= 16; ++parameter) {
        const auto prefix = QString("parameter%1").arg(parameter);
        if (ControlObject::get(ConfigKey(group, prefix + "_units")) != 1) continue;
        found = true;
        ControlObject::set(ConfigKey(group, prefix + "_beat_period"), 0.125);
        EXPECT_DOUBLE_EQ(0.125, ControlObject::get(ConfigKey(group, prefix + "_value")));
        EXPECT_DOUBLE_EQ(0.125, ControlObject::get(ConfigKey(group, prefix + "_beat_period")));
    }
    EXPECT_TRUE(found);
}

TEST_F(EffectSlotTest, CatalogueUpgradePreservesSavedFilesAndReplacesStaleFactoryCopy) {
    const auto directory = QDir(config()->getSettingsPath()).filePath("effects/chains/");
    ASSERT_TRUE(QDir().mkpath(directory));
    QFile source(config()->getResourcePath() + "effects/rekordbox7/02_ECHO.xml");
    ASSERT_TRUE(source.open(QIODevice::ReadOnly));
    const auto original = source.readAll();
    auto custom = original;
    custom.replace("[RB7] ECHO", "My Live Echo");
    QFile customFile(directory + "custom.xml");
    ASSERT_TRUE(customFile.open(QIODevice::WriteOnly));
    customFile.write(custom);
    customFile.close();
    auto stale = original;
    stale.replace("<Value>0.6</Value>", "<Value>0.1</Value>");
    QFile staleFile(directory + "stale-factory.xml");
    ASSERT_TRUE(staleFile.open(QIODevice::WriteOnly));
    staleFile.write(stale);
    staleFile.close();
    // Exercise a saved old list, and repeat startup to catch accumulating entries.
    QFile state(QDir(config()->getSettingsPath()).filePath("effects.xml"));
    ASSERT_TRUE(state.open(QIODevice::WriteOnly));
    state.write("<MixxxEffects><ChainPresetList><ChainPresetName>My Live Echo</ChainPresetName>"
                "<ChainPresetName>[RB7] ECHO</ChainPresetName></ChainPresetList></MixxxEffects>");
    state.close();
    for (int startup = 0; startup < 2; ++startup) {
        auto factory = std::make_shared<ChannelHandleFactory>();
        EffectsManager manager(config(), factory);
        ChannelHandleAndGroup output(factory->getOrCreateHandle("[MasterOutput]"), "[MasterOutput]");
        ChannelHandleAndGroup deck(factory->getOrCreateHandle("[Channel1]"), "[Channel1]");
        manager.registerOutputChannel(output);
        manager.registerInputChannel(output);
        manager.registerInputChannel(deck);
        manager.setup();
        const auto presets = manager.getChainPresetManager();
        int factoryCount = 0;
        for (const auto& preset : presets->getPresetsSorted()) {
            factoryCount += preset->isRekordbox7();
        }
        EXPECT_EQ(factoryCount, 25);
        EXPECT_EQ(presets->getPresetsSorted()[1]->name(), "[RB7] DELAY");
        ASSERT_TRUE(presets->getPreset("My Live Echo"));
        const auto echo = presets->getPreset("[RB7] ECHO");
        ASSERT_TRUE(echo);
        bool checked = false;
        for (const auto& parameter : echo->effectPresets()[0]->getParameterPresets()) {
            if (parameter.id() == "feedback_amount") {
                EXPECT_DOUBLE_EQ(parameter.value(), 0.6);
                checked = true;
            }
        }
        EXPECT_TRUE(checked);
    }
    ASSERT_TRUE(customFile.open(QIODevice::ReadOnly));
    EXPECT_EQ(customFile.readAll(), custom);
    ASSERT_TRUE(staleFile.open(QIODevice::ReadOnly));
    EXPECT_EQ(staleFile.readAll(), stale);
}

TEST_F(EffectSlotTest, StandardPickerPopulatesAndOpensTwoColumnDialog) {
    auto factory = std::make_shared<ChannelHandleFactory>();
    EffectsManager manager(config(), factory);
    ChannelHandleAndGroup output(factory->getOrCreateHandle("[MasterOutput]"), "[MasterOutput]");
    ChannelHandleAndGroup deck(factory->getOrCreateHandle("[Channel1]"), "[Channel1]");
    manager.registerOutputChannel(output);
    manager.registerInputChannel(output);
    manager.registerInputChannel(deck);
    manager.setup();
    QWidget window;
    window.resize(1024, 600);
    WEffectChainPresetSelector selector(&window, &manager);
    selector.setGeometry(852, 100, 164, 44);
    window.show();
    QDomDocument doc;
    ASSERT_TRUE(doc.setContent(QStringLiteral("<EffectChainPresetSelector><EffectUnitGroup>[EffectRack1_EffectUnit1]</EffectUnitGroup></EffectChainPresetSelector>")));
    SkinContext context(config(), QString());
    selector.setup(doc.documentElement(), context);
    ASSERT_GT(selector.count(), 25);
    EXPECT_EQ(selector.itemText(1), QStringLiteral("DELAY"));
    manager.getStandardEffectChain(0)->loadChainPreset(
            manager.getChainPresetManager()->getPreset("[RB7] ECHO"));
    bool opened = false;
    QTimer::singleShot(50, &selector, [&]() {
        auto* dialog = window.findChild<QDialog*>(QStringLiteral("BeatFxPicker"));
        if (dialog) {
            opened = dialog->isVisible();
            EXPECT_FALSE(dialog->isWindow());
            EXPECT_EQ(dialog->geometry(), QRect(852, 100, 164, 500));
            int entries = 0;
            QSet<int> columns, rows;
            QPushButton* next = nullptr;
            QPushButton* erase = nullptr;
            for (auto* button : dialog->findChildren<QPushButton*>()) {
                if (button->property("presetIndex").isValid()) {
                    ++entries;
                    EXPECT_EQ(button->height(), 44);
                    columns.insert(button->x());
                    rows.insert(button->y());
                } else {
                    EXPECT_EQ(button->height(), 30);
                    if (!button->accessibleName().isEmpty()) {
                        EXPECT_TRUE(button->icon().isNull());
                        EXPECT_EQ(button->text(), button->accessibleName());
                    }
                }
                if (button->text() == QStringLiteral("Erase")) erase = button;
                if (button->accessibleName() == QStringLiteral("Next")) {
                    next = button;
                }
            }
            EXPECT_EQ(entries, 14);
            EXPECT_EQ(columns.size(), 2);
            EXPECT_EQ(rows.size(), 7);
            const auto before = manager.getStandardEffectChain(0)->presetName();
            ASSERT_TRUE(next);
            next->click();
            EXPECT_EQ(manager.getStandardEffectChain(0)->presetName(), before);
            for (auto* button : dialog->findChildren<QPushButton*>()) {
                if (button->text() == QStringLiteral("Saved")) {
                    button->click();
                    break;
                }
            }
            for (auto* button : dialog->findChildren<QPushButton*>()) {
                if (button->isVisible() && button->property("presetIndex").isValid()) {
                    EXPECT_NE(selector.itemData(button->property("presetIndex").toInt()).toString(), kNoEffectString);
                    EXPECT_NE(button->text(), kNoEffectString);
                }
            }
            const int reverseRoll = selector.findText(QStringLiteral("7. REV ROLL"));
            ASSERT_GE(reverseRoll, 0);
            bool foundReverseRoll = false;
            for (auto* button : dialog->findChildren<QPushButton*>()) {
                if (button->property("presetIndex").isValid() &&
                        button->property("presetIndex").toInt() == reverseRoll) {
                    EXPECT_EQ(button->text(), QStringLiteral("7. REV\nROLL"));
                    foundReverseRoll = true;
                }
            }
            EXPECT_TRUE(foundReverseRoll);
            next->click();
            int savedPageTwoEntries = 0;
            for (auto* button : dialog->findChildren<QPushButton*>()) {
                if (button->isVisible() && button->property("presetIndex").isValid()) {
                    ++savedPageTwoEntries;
                    EXPECT_NE(selector.itemData(button->property("presetIndex").toInt()).toString(), kNoEffectString);
                }
            }
            EXPECT_EQ(savedPageTwoEntries, 8);
            EXPECT_EQ(manager.getStandardEffectChain(0)->presetName(), before);
            ASSERT_TRUE(erase);
            erase->click();
            EXPECT_FALSE(dialog->isVisible());
            EXPECT_FALSE(manager.getStandardEffectChain(0)->getEffectSlot(0)->isLoaded());
            EXPECT_EQ(manager.getStandardEffectChain(0)->presetName(), kNoEffectString);
            EXPECT_TRUE(manager.getChainPresetManager()->getPreset("[RB7] ECHO"));
        }
    });
    selector.showPopup();
    EXPECT_TRUE(opened);
}

TEST_F(EffectSlotTest, AvailableBeatButtonsFollowAllStandardPresetsAndClear) {
    auto factory = std::make_shared<ChannelHandleFactory>();
    EffectsManager manager(config(), factory);
    ChannelHandleAndGroup output(factory->getOrCreateHandle("[MasterOutput]"), "[MasterOutput]");
    ChannelHandleAndGroup deck(factory->getOrCreateHandle("[Channel1]"), "[Channel1]");
    manager.registerOutputChannel(output);
    manager.registerInputChannel(output);
    manager.registerInputChannel(deck);
    manager.setup();
    auto chain = manager.getStandardEffectChain(0);
    const auto group = chain->getEffectSlot(0)->getGroup();
    SkinContext context(config(), "test");
    std::vector<std::unique_ptr<WBeatPeriodPicker>> pickers;
    for (int parameter = 1; parameter <= 16; ++parameter) {
        QDomDocument doc;
        ASSERT_TRUE(doc.setContent(QString("<BeatPeriodPicker><EffectGroup>%1</EffectGroup><Parameter>%2</Parameter></BeatPeriodPicker>").arg(group).arg(parameter)));
        auto picker = std::make_unique<WBeatPeriodPicker>();
        picker->setup(doc.documentElement(), context);
        pickers.push_back(std::move(picker));
    }
    int reviewed = 0;
    for (const auto& preset : manager.getChainPresetManager()->getPresetsSorted()) {
        if (!preset->name().startsWith("[RB7] ")) continue;
        SCOPED_TRACE(preset->name().toStdString());
        chain->loadChainPreset(preset);
        QCoreApplication::processEvents();
        const auto id = chain->getEffectSlot(0)->getManifest()->id();
        const int expected = id.endsWith(".echo") || id.endsWith(".phaser") ? 5 :
                id.endsWith(".tremolo") ? 6 : 0;
        int visible = 0;
        for (int slot = 0; slot < 16; ++slot) {
            const auto prefix = QString("parameter%1").arg(slot + 1);
            for (int index = 0; index < 6; ++index) {
                auto* button = pickers[slot]->findChild<QPushButton*>(QString("BeatPeriod%1").arg(index));
                ASSERT_TRUE(button);
                if (button->isHidden()) continue;
                ++visible;
                EXPECT_TRUE(button->isEnabled());
                EXPECT_EQ(1, ControlObject::get(ConfigKey(group, prefix + "_loaded")));
                EXPECT_EQ(1, ControlObject::get(ConfigKey(group, prefix + "_units")));
                button->click();
                constexpr double periods[] = {0.125, 0.25, 0.5, 1, 2, 4};
                const double period = periods[index];
                const double raw = ControlObject::get(ConfigKey(group, prefix + "_value"));
                EXPECT_DOUBLE_EQ(period, ControlObject::get(ConfigKey(group, prefix + "_beat_period")));
                // Apply the actual native quantizer rules for these presets.
                const double nativePeriod = id.endsWith(".echo") ? std::max(std::round(raw * 4) / 4, 0.125) :
                        id.endsWith(".phaser") ? std::max(std::round(raw * 2) / 2, 0.25) : 1 / raw;
                EXPECT_DOUBLE_EQ(period, nativePeriod);
            }
        }
        EXPECT_EQ(expected, visible);
        bool hasLinkedKnob = false;
        for (const auto& effect : preset->effectPresets()) {
            for (const auto& parameter : effect->getParameterPresets()) {
                hasLinkedKnob |= parameter.linkType() != EffectManifestParameter::LinkType::None && !parameter.hidden();
            }
        }
        EXPECT_EQ(hasLinkedKnob ? 1 : 0, ControlObject::get(ConfigKey(chain->group(), "super1_available")));
        ++reviewed;
    }
    EXPECT_EQ(25, reviewed);
    chain->loadChainPreset(manager.getChainPresetManager()->getPreset("[RB7] ECHO"));
    for (int parameter = 1; parameter <= 16; ++parameter) {
        if (ControlObject::get(ConfigKey(group, QString("parameter%1_loaded").arg(parameter))) > 0) {
            ControlObject::set(ConfigKey(group, QString("parameter%1_link_type").arg(parameter)), 0);
        }
    }
    EXPECT_EQ(0, ControlObject::get(ConfigKey(chain->group(), "super1_available")));
    ControlObject::set(ConfigKey(group, "parameter1_link_type"), 1);
    EXPECT_EQ(1, ControlObject::get(ConfigKey(chain->group(), "super1_available")));
    ControlObject::set(ConfigKey(chain->group(), "clear"), 1);
    QCoreApplication::processEvents();
    EXPECT_EQ(0, ControlObject::get(ConfigKey(chain->group(), "super1_available")));
    for (const auto& picker : pickers) {
        for (auto* button : picker->findChildren<QPushButton*>()) {
            EXPECT_TRUE(button->isHidden());
            EXPECT_FALSE(button->isEnabled());
        }
    }
}

TEST_F(EffectSlotTest, MissingBackendPresetsAreHiddenAndSkippedWithoutDeletingFiles) {
    const QString directory = QDir(config()->getSettingsPath()).filePath("effects/chains");
    ASSERT_TRUE(QDir().mkpath(directory));
    const QString fileName = QDir(directory).filePath("Missing Backend Test.xml");
    QFile file(fileName);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    const QByteArray xml = "<EffectChain><Name>Missing Backend Test</Name><MixMode>DRY/WET</MixMode>"
            "<Effects><Effect><Id>org.mixxx.effects.echo</Id><BackendType>Built-In</BackendType></Effect>"
            "<Effect><Id>org.example.not-installed</Id><BackendType>Built-In</BackendType></Effect>"
            "</Effects></EffectChain>";
    file.write(xml);
    file.close();
    auto factory = std::make_shared<ChannelHandleFactory>();
    EffectsManager manager(config(), factory);
    ChannelHandleAndGroup output(factory->getOrCreateHandle("[MasterOutput]"), "[MasterOutput]");
    ChannelHandleAndGroup deck(factory->getOrCreateHandle("[Channel1]"), "[Channel1]");
    manager.registerOutputChannel(output);
    manager.registerInputChannel(output);
    manager.registerInputChannel(deck);
    manager.setup();
    auto presets = manager.getChainPresetManager();
    auto unavailable = presets->getPreset("Missing Backend Test");
    ASSERT_TRUE(unavailable);
    EXPECT_FALSE(presets->isPresetAvailable(unavailable));
    WEffectChainPresetSelector selector(nullptr, &manager);
    QDomDocument doc;
    ASSERT_TRUE(doc.setContent(QStringLiteral("<EffectChainPresetSelector><EffectUnitGroup>[EffectRack1_EffectUnit1]</EffectUnitGroup></EffectChainPresetSelector>")));
    SkinContext context(config(), "test");
    selector.setup(doc.documentElement(), context);
    EXPECT_EQ(-1, selector.findData(QStringLiteral("Missing Backend Test")));
    auto chain = manager.getStandardEffectChain(0);
    for (int direction : {1, -1}) {
        QSet<QString> visited;
        for (int step = 0; step < presets->numPresets(); ++step) {
            ControlObject::set(ConfigKey(chain->group(), "chain_selector"), direction);
            EXPECT_NE(QStringLiteral("Missing Backend Test"), chain->presetName());
            visited.insert(chain->presetName());
        }
        EXPECT_EQ(presets->numPresets() - 1, visited.size());
    }
    ASSERT_TRUE(file.open(QIODevice::ReadOnly));
    EXPECT_EQ(xml, file.readAll());
}

// Every available Standard/Saved preset uses the same metadata, including
// reordered parameter slots. Clearing/replacing an effect must clear the flag.
TEST_F(EffectSlotTest, InternalMixMetadataFollowsAllAvailablePresets) {
    auto factory = std::make_shared<ChannelHandleFactory>();
    EffectsManager manager(config(), factory);
    ChannelHandleAndGroup output(factory->getOrCreateHandle("[MasterOutput]"), "[MasterOutput]");
    ChannelHandleAndGroup deck(factory->getOrCreateHandle("[Channel1]"), "[Channel1]");
    manager.registerOutputChannel(output);
    manager.registerInputChannel(output);
    manager.registerInputChannel(deck);
    manager.setup();
    auto chain = manager.getStandardEffectChain(0);
    auto effect = chain->getEffectSlot(0);
    const auto group = effect->getGroup();
    int mixParameters = 0;
    auto checkSlots = [&] {
        manager.getEngineEffectsManager()->onCallbackStart();
        QCoreApplication::processEvents();
        for (int index = 0; index < 16; ++index) {
            auto slot = effect->getEffectParameterSlot(EffectParameterType::Knob, index);
            ASSERT_TRUE(slot);
            const auto manifest = slot->getManifest();
            const bool isMix = manifest && (manifest->id() == "mix" || manifest->id() == "dry_wet");
            EXPECT_EQ(isMix ? 1 : 0, ControlObject::get(ConfigKey(group,
                    QString("parameter%1_is_mix").arg(index + 1))));
            mixParameters += isMix;
        }
    };
    // Include available backends beyond the Standard catalogue (White Noise,
    // for example), then exercise saved ordering and effect replacement.
    for (const auto& manifest : manager.getBackendManager()->getManifests()) {
        SCOPED_TRACE(manifest->id().toStdString());
        effect->loadEffectWithDefaults(manifest);
        checkSlots();
    }
    for (const auto& preset : manager.getChainPresetManager()->getPresetsSorted()) {
        if (!manager.getChainPresetManager()->isPresetAvailable(preset)) continue;
        SCOPED_TRACE(preset->name().toStdString());
        chain->loadChainPreset(preset);
        checkSlots();
    }
    EXPECT_GT(mixParameters, 0);
    ControlObject::set(ConfigKey(chain->group(), "clear"), 1);
    for (int index = 1; index <= 16; ++index) {
        EXPECT_EQ(0, ControlObject::get(ConfigKey(group,
                QString("parameter%1_is_mix").arg(index))));
    }
}
