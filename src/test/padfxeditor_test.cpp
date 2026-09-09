#include "test/mixxxtest.h"
#include "preferences/padfxsettings.h"
#include "skin/legacy/skincontext.h"
#include "widget/wpadfxeditor.h"
#include <QDomNode>

class PadFxEditorTest : public MixxxTest {};
TEST_F(PadFxEditorTest, FitsSmallScreenAndKeepsTouchControlsVisible) {
    PadFxSettings settings(config());
    WPadFxEditor editor;
    SkinContext context(config(), "");
    editor.setup(QDomNode(), context);
    editor.resize(1024, 420); // Space left below both navigation bars and footer.
    editor.show();
    QCoreApplication::processEvents();
    EXPECT_LE(editor.minimumSizeHint().height(), 420);
    EXPECT_LE(editor.minimumSizeHint().width(), 1024);
    auto* effect = editor.findChild<QComboBox*>("PadFxPad1Field0");
    ASSERT_NE(effect, nullptr);
    effect->showPopup();
    QCoreApplication::processEvents();
    EXPECT_LE(effect->view()->window()->height(), 420);
    effect->hidePopup();
    auto* deck = editor.findChild<QComboBox*>("PadFxDeck");
    auto* bank = editor.findChild<QComboBox*>("PadFxBank");
    ASSERT_NE(deck, nullptr);
    ASSERT_NE(bank, nullptr);
    EXPECT_EQ(deck->count(), 2);
    for (int d = 0; d < 2; ++d) {
        deck->setCurrentIndex(d);
        for (int b = 0; b < 2; ++b) {
            bank->setCurrentIndex(b);
            for (auto* button : editor.findChildren<QPushButton*>()) {
                if (button->isCheckable()) button->click();
                QCoreApplication::processEvents();
                for (auto* control : editor.findChildren<QWidget*>()) {
                    if (!control->isVisible() || (!qobject_cast<QComboBox*>(control) &&
                            !qobject_cast<QPushButton*>(control))) continue;
                    const QRect bounds(control->mapTo(&editor, QPoint()), control->size());
                    EXPECT_TRUE(editor.rect().contains(bounds)) << control->objectName().toStdString();
                    EXPECT_GE(control->height(), 44);
                }
            }
        }
    }
}

TEST_F(PadFxEditorTest, DaylightLabelUsesDarkForeground) {
    config()->setValue(ConfigKey("[BiteDJ]", "high_contrast"), 1);
    HighContrast contrast(config());
    PadFxSettings settings(config());
    WPadFxEditor editor;
    SkinContext context(config(), "");
    editor.setup(QDomNode(), context);
    editor.resize(1024, 500);
    editor.show();
    QCoreApplication::processEvents();
    auto* card = editor.findChild<QFrame*>("PadFxCard1");
    ASSERT_NE(card, nullptr);
    auto* label = card->findChild<QLabel*>();
    ASSERT_NE(label, nullptr);
    EXPECT_EQ(label->palette().color(QPalette::WindowText),
            QColor(HighContrast::mapColor(QColor("#ff8a81")).name()));
    const QRect bounds(card->mapTo(&editor, QPoint()), card->size());
    EXPECT_EQ(bounds.bottom(), editor.height() - 13);
    EXPECT_GE(bounds.left(), 16);
    EXPECT_LE(bounds.right(), editor.width() - 17);
}
