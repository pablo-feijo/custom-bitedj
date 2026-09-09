#include <gtest/gtest.h>
#include <QDomDocument>
#include "control/controlobject.h"
#include "skin/legacy/skincontext.h"
#include "test/mixxxtest.h"
#include "widget/wnumberpos.h"

class DeckPresentationTest : public MixxxTest {};

TEST_F(DeckPresentationTest, PerDeckTimeModesAreIndependentAndRefreshWhenPaused) {
    ControlObject global(ConfigKey("[Controls]", "ShowDurationRemaining"));
    ControlObject format(ConfigKey("[Controls]", "TimeFormat"));
    ControlObject mode1(ConfigKey("[Skin]", "deck1_time_mode"));
    ControlObject mode2(ConfigKey("[Skin]", "deck2_time_mode"));
    ControlObject elapsed1(ConfigKey("[Channel1]", "time_elapsed"));
    ControlObject remain1(ConfigKey("[Channel1]", "time_remaining"));
    ControlObject elapsed2(ConfigKey("[Channel2]", "time_elapsed"));
    ControlObject remain2(ConfigKey("[Channel2]", "time_remaining"));
    elapsed1.set(12); remain1.set(48); elapsed2.set(20); remain2.set(40);
    WNumberPos first("[Channel1]"), second("[Channel2]");
    SkinContext context(config(), "test");
    QDomDocument xml1, xml2;
    ASSERT_TRUE(xml1.setContent(QStringLiteral("<NumberPos><ModeConfigKey>[Skin],deck1_time_mode</ModeConfigKey></NumberPos>")));
    ASSERT_TRUE(xml2.setContent(QStringLiteral("<NumberPos><ModeConfigKey>[Skin],deck2_time_mode</ModeConfigKey></NumberPos>")));
    first.setup(xml1.documentElement(), context);
    second.setup(xml2.documentElement(), context);
    mode1.set(1); mode2.set(0);
    EXPECT_TRUE(first.text().startsWith('-'));
    EXPECT_FALSE(second.text().startsWith('-'));
    const QString secondBefore = second.text();
    mode1.set(0);
    EXPECT_FALSE(first.text().startsWith('-'));
    EXPECT_EQ(second.text(), secondBefore);
    global.set(1);
    EXPECT_FALSE(first.text().startsWith('-'));
    EXPECT_EQ(second.text(), secondBefore);
    elapsed1.set(0); mode1.set(1); remain1.set(125);
    EXPECT_TRUE(first.text().contains("2:05"));
}

#include <QApplication>
#include <QImage>
#include <QThread>
#include "widget/wlabel.h"

TEST_F(DeckPresentationTest, LongTitleScrollPreservesFullTextAndGeometry) {
    WLabel title;
    SkinContext context(config(), "test");
    QDomDocument xml;
    ASSERT_TRUE(xml.setContent(QStringLiteral("<Label><Elide>scroll</Elide></Label>")));
    title.setup(xml.documentElement(), context);
    title.resize(120, 24);
    const QString text = QStringLiteral("A long Unicode café track title with literal <brackets> and more words");
    title.setText(text);
    title.show();
    const auto before = title.grab().toImage();
    for (int i = 0; i < 38; ++i) {
        QThread::msleep(55);
        QApplication::processEvents();
    }
    EXPECT_EQ(title.text(), text);
    EXPECT_EQ(title.size(), QSize(120, 24));
    EXPECT_NE(before, title.grab().toImage());
    title.setText(QStringLiteral("Short"));
    EXPECT_EQ(title.text(), QStringLiteral("Short"));
}

#include "waveform/renderers/waveformsignalcolors.h"

TEST_F(DeckPresentationTest, AmberPaletteIsOptInAndOriginalColorsRemainAvailable) {
    SkinContext context(config(), "test");
    QDomDocument xml;
    ASSERT_TRUE(xml.setContent(QStringLiteral("<Visual><BiteDJPalette>true</BiteDJPalette><SignalColor>#222222</SignalColor><SignalLowColor>#123456</SignalLowColor><SignalMidColor>#234567</SignalMidColor><SignalHighColor>#345678</SignalHighColor></Visual>")));
    config()->setValue(ConfigKey("[BiteDJ]", "waveform_palette"), 0);
    WaveformSignalColors original;
    original.setup(xml.documentElement(), context);
    EXPECT_EQ(original.getLowColor(), QColor("#123456"));
    config()->setValue(ConfigKey("[BiteDJ]", "waveform_palette"), 1);
    WaveformSignalColors amber;
    amber.setup(xml.documentElement(), context);
    EXPECT_EQ(amber.getLowColor(), QColor("#0055e1"));
    EXPECT_EQ(amber.getMidColor(), QColor("#b4690a"));
    EXPECT_EQ(amber.getHighColor(), QColor("#f5ebd7"));
    EXPECT_EQ(amber.getRgbLowColor(), amber.getLowColor());
    xml.documentElement().removeChild(xml.documentElement().firstChildElement("BiteDJPalette"));
    WaveformSignalColors otherSkin;
    otherSkin.setup(xml.documentElement(), context);
    EXPECT_EQ(otherSkin.getLowColor(), original.getLowColor());
}
