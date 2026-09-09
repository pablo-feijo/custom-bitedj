#pragma once

#include <QEvent>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QVector>

#include "control/controlproxy.h"
#include "skin/highcontrast.h"
#include "skin/legacy/skincontext.h"
#include "widget/wbasewidget.h"

// Native buttons provide press/release and hold-repeat semantics for grid edits.
class WBeatGridControls : public QWidget, public WBaseWidget {
  public:
    explicit WBeatGridControls(QWidget* parent = nullptr)
            : QWidget(parent), WBaseWidget(this) {
    }

    void setup(const QDomNode& node, const SkinContext& context) {
        const int deck = context.selectInt(node, "Channel");
        if (deck < 1 || deck > 2) return;
        const QString group = QStringLiteral("[Channel%1]").arg(deck);
        auto* layout = new QGridLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(8);
        auto* title = new QLabel(tr("Deck %1 · Grid").arg(deck), this);
        title->setObjectName(QStringLiteral("GridDeckLabel"));
        title->setFixedHeight(28);
        layout->addWidget(title, 0, 0, 1, 6);

        const auto add = [this, layout, &group, deck](const QString& label,
                                 const QString& command, const QString& description,
                                 int row, int column, int span, bool repeat) {
            auto* button = new QPushButton(label, this);
            button->setObjectName(command);
            button->setAccessibleName(tr("Deck %1: %2").arg(deck).arg(description));
            button->setToolTip(description);
            button->setFocusPolicy(Qt::NoFocus);
            button->setMinimumSize(44, 48);
            button->setAutoRepeat(repeat);
            button->setAutoRepeatDelay(350);
            button->setAutoRepeatInterval(80);
            auto* control = new ControlProxy(group, command, this);
            connect(button, &QPushButton::pressed, this, [control] { control->set(1); });
            connect(button, &QPushButton::released, this, [control] { control->set(0); });
            m_actions.append({button, control});
            layout->addWidget(button, row, column, 1, span);
        };
        add(tr("Earlier"), "beats_translate_earlier", tr("Shift grid earlier"), 1, 0, 2, true);
        add(tr("Set"), "beats_translate_curpos", tr("Set grid at current position"), 1, 2, 2, false);
        add(tr("Later"), "beats_translate_later", tr("Shift grid later"), 1, 4, 2, true);
        add(tr("BPM −"), "beats_adjust_slower", tr("Slower grid BPM"), 2, 0, 3, true);
        add(tr("BPM +"), "beats_adjust_faster", tr("Faster grid BPM"), 2, 3, 3, true);
        for (int column = 0; column < 6; ++column) layout->setColumnStretch(column, 1);
        setStyleSheet(HighContrast::mapStyleSheet(QStringLiteral(
                "QLabel { color:#a7a9ac; font-size:13px; font-weight:bold; }"
                "QPushButton { background:#1a1a1a; color:#a7a9ac; border:1px solid #3a3a3a;"
                " border-radius:3px; font-size:12px; font-weight:bold; padding:2px; }"
                "QPushButton:pressed { border-color:#855ea7; color:#e7e8e9; }")));
    }

  protected:
    bool event(QEvent* event) override {
        if (event->type() == QEvent::Hide || event->type() == QEvent::WindowDeactivate ||
                (event->type() == QEvent::EnabledChange && !isEnabled())) {
            for (const auto& action : m_actions) {
                action.button->setDown(false);
                action.control->set(0);
            }
        }
        return QWidget::event(event);
    }

  private:
    struct Action {
        QPushButton* button;
        ControlProxy* control;
    };
    QVector<Action> m_actions;
};
