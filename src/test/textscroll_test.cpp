#include "test/mixxxtest.h"
#include "widget/textscroll.h"
#include "widget/wlabel.h"
#include "skin/legacy/skincontext.h"
#include <QDomDocument>

class TextScrollTest : public MixxxTest {};
TEST_F(TextScrollTest, PausesTravelsAndRestartsWithoutOvershoot) {
    EXPECT_DOUBLE_EQ(mixxx::textScrollOffset(1000, 90, 30), 0);
    EXPECT_DOUBLE_EQ(mixxx::textScrollOffset(2500, 90, 30), 30);
    EXPECT_DOUBLE_EQ(mixxx::textScrollOffset(5000, 90, 30), 90);
    EXPECT_DOUBLE_EQ(mixxx::textScrollOffset(6000, 90, 30), 0);
    EXPECT_DOUBLE_EQ(mixxx::textScrollOffset(5000, 0, 30), 0);
}
TEST_F(TextScrollTest, TimerStopsWhenHiddenOrTextFitsAndDoesNotGrowDeck) {
    WLabel label;
    SkinContext context(config(), "");
    QDomDocument doc;
    ASSERT_TRUE(doc.setContent(QString("<Label><Elide>scroll</Elide></Label>")));
    label.setup(doc.documentElement(), context);
    label.resize(160, 32);
    label.setText(QString(200, QChar('W')));
    label.show();
    QCoreApplication::processEvents();
    auto* timer = label.findChild<QTimer*>("TextScrollTimer");
    ASSERT_NE(timer, nullptr);
    EXPECT_TRUE(timer->isActive());
    EXPECT_EQ(label.sizeHint().width(), 0);
    EXPECT_EQ(label.accessibleName(), label.text());
    label.hide();
    EXPECT_FALSE(timer->isActive());
    label.show();
    EXPECT_TRUE(timer->isActive());
    label.setText("Short");
    EXPECT_FALSE(timer->isActive());
    EXPECT_EQ(static_cast<QLabel*>(&label)->text(), "Short");
}

TEST_F(TextScrollTest, OverflowingTextUsesDaylightSkinColor) {
    WLabel label;
    SkinContext context(config(), "");
    QDomDocument doc;
    ASSERT_TRUE(doc.setContent(QString("<Label><Elide>scroll</Elide></Label>")));
    label.setup(doc.documentElement(), context);
    label.setStyleSheet("WLabel { background: #ffffff; color: #18191c; qproperty-scrollColor: #18191c; }");
    label.resize(160, 32);
    label.setText(QString(100, QChar('W')));
    label.show();
    QCoreApplication::processEvents();
    const auto picture = label.grab().toImage();
    int darkPixels = 0;
    for (int y = 0; y < picture.height(); ++y) {
        for (int x = 0; x < picture.width(); ++x) {
            if (picture.pixelColor(x, y).lightness() < 80) ++darkPixels;
        }
    }
    EXPECT_GT(darkPixels, 30);
}
