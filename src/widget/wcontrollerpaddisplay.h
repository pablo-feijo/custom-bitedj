#pragma once

#include <array>
#include <cmath>
#include <QGridLayout>
#include <QLabel>
#include <QStringList>

#include "control/controlproxy.h"
#include "skin/highcontrast.h"
#include "skin/legacy/skincontext.h"
#include "widget/wbasewidget.h"

// Read-only controller legend. Assignments and playback remain owned by the
// system and controller mapping; opening the drawer never changes either.
class WControllerPadDisplay : public QWidget, public WBaseWidget {
  public:
    explicit WControllerPadDisplay(QWidget* parent = nullptr)
            : QWidget(parent), WBaseWidget(this) {
        auto* grid = new QGridLayout(this);
        grid->setContentsMargins(0, 0, 0, 0);
        grid->setSpacing(4);
        for (int pad = 0; pad < 8; ++pad) {
            auto* label = new QLabel(this);
            label->setObjectName(QStringLiteral("ControllerPadLegend"));
            label->setAlignment(Qt::AlignCenter);
            label->setWordWrap(true);
            label->setMinimumHeight(44);
            label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);
            m_labels[pad] = label;
            grid->addWidget(label, pad / 4, pad % 4);
        }
        for (int col = 0; col < 4; ++col) grid->setColumnStretch(col, 1);
    }

    void setup(const QDomNode& node, const SkinContext& context) {
        const int deck = context.selectInt(node, "Channel");
        if (deck < 1 || deck > 2) return;
        const auto prefix = QStringLiteral("d%1_").arg(deck);
        m_mode = watch(prefix + "mode");
        m_shift = watch(prefix + "shift");
        m_jumpBank = watch(prefix + "jump_bank");
        const QStringList fields = {"effect", "beat", "strength", "hold"};
        for (int slot = 0; slot < 16; ++slot) {
            for (int field = 0; field < 4; ++field) {
                m_slots[slot][field] = watch(prefix + QStringLiteral("s%1_%2")
                        .arg(slot).arg(fields[field]));
            }
        }
        setStyleSheet(HighContrast::mapStyleSheet(QStringLiteral(
                "QLabel#ControllerPadLegend { background:#20232c; color:#edf0fa;"
                " border:1px solid #565b6b; border-radius:3px; font-size:14px; padding:4px 8px; }")));
        m_ready = true;
        refresh();
    }

  private:
    ControlProxy* watch(const QString& name) {
        auto* control = new ControlProxy(QStringLiteral("[PadFX]"), name, this);
        control->connectValueChanged(this, [this](double) { refresh(); });
        return control;
    }
    static int state(ControlProxy* control, int count, int fallback = 0) {
        const double value = control->get();
        return std::isfinite(value) && value >= 0 && value < count && value == std::floor(value)
                ? static_cast<int>(value) : fallback;
    }
    void refresh() {
        if (!m_ready) return;
        const int mode = state(m_mode, 5);
        const bool shifted = m_shift->get() != 0;
        const QStringList effects = {tr("Roll 1/2"), tr("Sweep"), tr("Flanger 16"),
                tr("Release Brake 3/4"), tr("Echo 1/4"), tr("Echo 1/2"), tr("Reverb"),
                tr("Release Echo 1/2"), tr("Trans 1/2"), tr("Crush"), tr("Filter LFO 4"),
                tr("Release Backspin 4"), tr("MT Delay 1/8 ≈"), tr("Dub Echo ≈"),
                tr("Space ≈"), tr("Release Echo 1"), tr("Off")};
        const QStringList beats = {"", "1/8", "1/4", "1/2", "3/4", "1", "2"};
        const QStringList loopSizes = {"1/4", "1/2", "1", "2", "4", "8", "16", "32"};
        for (int pad = 0; pad < 8; ++pad) {
            QString text;
            if (mode == 1 || (mode == 3 && shifted)) {
                // Shift+Beat Loop pads are mapped to the shifted Pad FX bank.
                const int slot = pad + (shifted ? 8 : 0);
                const int effect = state(m_slots[slot][0], 17, 16);
                const int beat = state(m_slots[slot][1], 7);
                const int strength = state(m_slots[slot][2], 5);
                const bool hold = m_slots[slot][3]->get() == 1;
                if (effect < 0 || effect >= effects.size() || strength == 0) {
                    text = tr("Off");
                } else {
                    text = effects[effect];
                    // Only echo-family lanes consume the timing override.
                    if ((effect == 4 || effect == 5 || effect == 7 || effect == 12 ||
                                effect == 13 || effect == 15) && beat > 0 && beat < beats.size()) {
                        const QStringList names = {tr("Echo"), tr("Release Echo"),
                                tr("MT Delay ≈"), tr("Dub Echo ≈")};
                        const int name = effect == 7 || effect == 15 ? 1 : effect == 12 ? 2 : effect == 13 ? 3 : 0;
                        text = names[name] + " " + beats[beat];
                    }
                    if (effect != 16) {
                        text += tr("\n%1% · %2").arg(strength * 25)
                                .arg(hold && (effect == 7 || effect == 15) ? tr("Toggle") : tr("Hold"));
                    }
                }
            } else if (mode == 2) {
                const double scale = m_jumpBank->get() == 0 ? 1.0 / 16 : m_jumpBank->get() == 2 ? 16 : 1;
                if (shifted) {
                    text = pad == 6 ? tr("Jump sizes ÷16") : pad == 7 ? tr("Jump sizes ×16") : tr("—");
                } else {
                    text = tr("%1 %2 beats").arg(pad % 2 ? QStringLiteral("→") : QStringLiteral("←"))
                            .arg(scale * (1 << (pad / 2)));
                }
            } else if (mode == 3) {
                text = loopSizes[pad] + (pad < 4 ? tr(" beat roll\nHold") : tr(" beat loop\nToggle"));
            }
            m_labels[pad]->setText(tr("%1 · %2").arg(pad + 1).arg(text));
        }
    }
    bool m_ready = false;
    ControlProxy* m_mode = nullptr;
    ControlProxy* m_shift = nullptr;
    ControlProxy* m_jumpBank = nullptr;
    std::array<QLabel*, 8> m_labels{};
    std::array<std::array<ControlProxy*, 4>, 16> m_slots{};
};
