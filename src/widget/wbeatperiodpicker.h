#pragma once

#include <QGridLayout>
#include <QPushButton>
#include <array>
#include <cmath>

#include "control/controlproxy.h"
#include "skin/highcontrast.h"
#include "skin/legacy/skincontext.h"
#include "widget/wbasewidget.h"

class WBeatPeriodPicker : public QWidget, public WBaseWidget {
  public:
    explicit WBeatPeriodPicker(QWidget* parent = nullptr)
            : QWidget(parent), WBaseWidget(this) {
    }
    void setup(const QDomNode& node, const SkinContext& context) {
        const QString group = context.selectString(node, "EffectGroup");
        const QString prefix = QStringLiteral("parameter%1").arg(context.selectInt(node, "Parameter"));
        const auto watch = [this, &group](const QString& item) {
            auto* control = new ControlProxy(group, item, this);
            control->connectValueChanged(this, [this](double) { refresh(); });
            return control;
        };
        m_period = watch(prefix + "_beat_period");
        m_minimum = watch(prefix + "_beat_period_min");
        m_maximum = watch(prefix + "_beat_period_max");
        m_loaded = watch(prefix + "_loaded");
        m_units = watch(prefix + "_units");
        auto* grid = new QGridLayout(this);
        grid->setContentsMargins(0, 0, 0, 0);
        grid->setSpacing(4);
        const QStringList labels = {"1/8", "1/4", "1/2", "1", "2", "4"};
        for (int i = 0; i < 6; ++i) {
            auto* button = new QPushButton(labels[i], this);
            button->setObjectName(QStringLiteral("BeatPeriod%1").arg(i));
            button->setAccessibleName(tr("%1 beats").arg(labels[i]));
            button->setMinimumSize(44, 28);
            button->setFocusPolicy(Qt::NoFocus);
            button->setCheckable(true);
            connect(button, &QPushButton::clicked, this, [this, i] {
                m_period->set(kPeriods[i]);
                refresh();
            });
            m_buttons[i] = button;
            grid->addWidget(button, i / 3, i % 3);
        }
        for (int column = 0; column < 3; ++column) grid->setColumnStretch(column, 1);
        setStyleSheet(HighContrast::mapStyleSheet(QStringLiteral(
                "QPushButton { background:#1a1a1a; color:#a7a9ac; border:1px solid #3a3a3a;"
                " border-radius:3px; font-size:13px; font-weight:bold; padding:2px; }"
                "QPushButton:checked { background:#855ea7; color:#e7e8e9; border-color:#855ea7; }"
                "QPushButton:disabled { color:#565b6b; }")));
        m_ready = true;
        refresh();
    }
  private:
    void refresh() {
        if (!m_ready) return;
        const double current = m_period->get();
        for (int i = 0; i < 6; ++i) {
            const bool available = m_loaded->get() == 1 && m_units->get() == 1 &&
                    kPeriods[i] >= m_minimum->get() && kPeriods[i] <= m_maximum->get();
            m_buttons[i]->setVisible(available);
            m_buttons[i]->setEnabled(available);
            m_buttons[i]->setChecked(std::abs(current - kPeriods[i]) < 1e-9);
        }
    }
    static constexpr std::array<double, 6> kPeriods = {0.125, 0.25, 0.5, 1, 2, 4};
    bool m_ready = false;
    ControlProxy* m_period = nullptr;
    ControlProxy* m_minimum = nullptr;
    ControlProxy* m_maximum = nullptr;
    ControlProxy* m_loaded = nullptr;
    ControlProxy* m_units = nullptr;
    std::array<QPushButton*, 6> m_buttons{};
};
