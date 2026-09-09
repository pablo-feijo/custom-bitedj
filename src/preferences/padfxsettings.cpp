#include "preferences/padfxsettings.h"

#include <cmath>

#include "control/controlobject.h"
#include "control/controlpushbutton.h"
#include "util/fpclassify.h"

namespace {
const QString kGroup = QStringLiteral("[PadFX]");
const QString kStorage = QStringLiteral("[PadFX_v1]");
// Stable BiteDJ IDs, NOT Rekordbox's proprietary enum values. Append catalog
// entries only; changing these contracts requires a new storage version.
constexpr const char* kNames[] = {"effect", "beat", "strength", "hold"};
constexpr int kStates[] = {17, 7, 5, 2}; // effect 16 = Off; strength 0..4 = 0..100%
int defaultValue(int slot, int field) {
    return field == 0 ? slot % 16 : field == 2 ? 4 : 0;
}
bool valid(double value, int states) {
    return util_isfinite(value) && value >= 0 && value < states &&
            value == std::floor(value);
}
} // namespace

PadFxSettings::PadFxSettings(UserSettingsPointer config)
        : m_config(std::move(config)) {
    for (int slot = 0; slot < 64; ++slot) {
        for (int field = 0; field < kFields; ++field) {
            const QString name = QStringLiteral("d%1_s%2_%3")
                                         .arg(slot / 16 + 1)
                                         .arg(slot % 16)
                                         .arg(QString::fromLatin1(kNames[field]));
            const ConfigKey key(kStorage, name);
            const int fallback = defaultValue(slot, field);
            const double stored = m_config->getValue<double>(key, fallback);
            auto* control = addControl(name, kStates[field],
                    valid(stored, kStates[field]) ? static_cast<int>(stored) : fallback);
            m_slots[slot][field] = control;
            m_values[slot][field] = static_cast<int>(control->get());
            connect(control, &ControlObject::valueChanged, this,
                    [this, field, slot](double value) { setSlot(slot, field, value); });
        }
    }
    // Runtime controller display state. Never persisted with assignments.
    for (int deck = 1; deck <= 2; ++deck) {
        const auto prefix = QStringLiteral("d%1_").arg(deck);
        auto* mode = addControl(prefix + "mode", 5, 0); // Existing IDs; append Memory=4.
        auto* performance = addControl(prefix + "performance_visible", 2, 0);
        auto* cycle = addControl(prefix + "cycle", 0, 0);
        connect(mode, &ControlObject::valueChanged, this, [mode, performance](double value) {
            if (!valid(value, 5)) {
                mode->set(0);
                performance->set(0);
                return;
            }
            performance->set(value > 0 && value < 4 ? 1 : 0);
        });
        connect(cycle, &ControlObject::valueChanged, this, [mode, performance](double value) {
            if (value != 1) return;
            // Touch order: Hot Cues -> Memory -> Beat Jump -> Pad FX -> Beat Loop.
            constexpr int next[] = {4, 3, 1, 0, 2};
            const double current = mode->get();
            const int selected = valid(current, 5) ? next[static_cast<int>(current)] : 0;
            // ControlObject suppresses callbacks to the object that made the write.
            // Publish the derived flag here too, not just on external mode changes.
            performance->set(selected > 0 && selected < 4 ? 1 : 0);
            mode->set(selected);
        });
        auto* previous = addControl(prefix + "previous", 0, 0);
        connect(previous, &ControlObject::valueChanged, this, [mode, performance](double value) {
            if (value != 1) return;
            constexpr int previousMode[] = {3, 2, 4, 1, 0};
            const double current = mode->get();
            const int selected = valid(current, 5) ? previousMode[static_cast<int>(current)] : 0;
            performance->set(selected > 0 && selected < 4 ? 1 : 0);
            mode->set(selected);
        });
        addControl(prefix + "shift", 2, 0);
        addControl(prefix + "jump_bank", 3, 1); // 1/16, 1, 16 multiplier
    }
    // Reset commands live with the system defaults, independent of any skin.
    for (int slot = 0; slot < 64; ++slot) {
        auto* reset = addControl(QStringLiteral("d%1_s%2_reset")
                .arg(slot / 16 + 1).arg(slot % 16), 0, 0);
        connect(reset, &ControlObject::valueChanged, this, [this, slot](double value) {
            if (value != 1) return;
            for (int field = 0; field < kFields; ++field) {
                setSlot(slot, field, defaultValue(slot, field));
            }
        });
    }
    m_deck = addControl(QStringLiteral("deck"), 4, 0);
    m_slot = addControl(QStringLiteral("slot"), 16, 0);
    for (int field = 0; field < kFields; ++field) {
        m_editor[field] = addControl(QString::fromLatin1(kNames[field]),
                kStates[field], defaultValue(0, field));
        connect(m_editor[field], &ControlObject::valueChanged, this,
                [this, field](double value) {
                    if (m_refreshing) {
                        return;
                    }
                    if (valid(value, kStates[field])) {
                        setSlot(selectedSlot(), field, value);
                    }
                    refreshEditor();
                });
    }
    const auto selectionChanged = [this](double) {
        // Reject malformed controller writes before using them as an index.
        if (!valid(m_deck->get(), 4)) {
            m_deck->set(0);
        }
        if (!valid(m_slot->get(), 16)) {
            m_slot->set(0);
        }
        refreshEditor();
    };
    connect(m_deck, &ControlObject::valueChanged, this, selectionChanged);
    connect(m_slot, &ControlObject::valueChanged, this, selectionChanged);
    auto* reset = addControl(QStringLiteral("reset_slot"), 0, 0);
    connect(reset, &ControlObject::valueChanged, this, [this](double value) {
        if (value != 1) {
            return;
        }
        const int slot = selectedSlot();
        for (int field = 0; field < kFields; ++field) {
            setSlot(slot, field, defaultValue(slot, field));
        }
    });
    // Mapping subscribes to this momentary emergency stop. It is never saved.
    addControl(QStringLiteral("clear_all"), 0, 0);
    auto* version = addControl(QStringLiteral("version"), 0, 1);
    version->setReadOnly();
    refreshEditor();
}

PadFxSettings::~PadFxSettings() = default;

void PadFxSettings::setSlot(int slot, int field, double value) {
    if (valid(value, kStates[field])) {
        m_values[slot][field] = static_cast<int>(value);
        const QString name = QStringLiteral("d%1_s%2_%3")
                                     .arg(slot / 16 + 1)
                                     .arg(slot % 16)
                                     .arg(QString::fromLatin1(kNames[field]));
        m_config->setValue(ConfigKey(kStorage, name), static_cast<int>(value));
    }
    // Creator-originated CO::set does not emit our own valueChanged signal.
    // Both the editor and direct mapping writes use this explicit commit path.
    m_slots[slot][field]->set(m_values[slot][field]);
    if (m_deck && m_slot && selectedSlot() == slot) {
        refreshEditor();
    }
}

ControlObject* PadFxSettings::addControl(const QString& item, int states, int initial) {
    auto control = std::make_unique<ControlPushButton>(ConfigKey(kGroup, item));
    if (states > 0) {
        control->setButtonMode(ControlPushButton::TOGGLE);
        control->setStates(states);
    }
    control->set(initial);
    auto* result = control.get();
    m_controls.push_back(std::move(control));
    return result;
}

int PadFxSettings::selectedSlot() const {
    // External CO writes can arrive before their queued validation callback.
    // Never trust a live double as an array index, even on the GUI thread.
    const double deck = m_deck->get();
    const double slot = m_slot->get();
    return (valid(deck, 4) ? static_cast<int>(deck) : 0) * 16 +
            (valid(slot, 16) ? static_cast<int>(slot) : 0);
}

void PadFxSettings::refreshEditor() {
    if (m_refreshing) {
        return;
    }
    m_refreshing = true;
    for (int field = 0; field < kFields; ++field) {
        m_editor[field]->set(m_slots[selectedSlot()][field]->get());
    }
    m_refreshing = false;
}
