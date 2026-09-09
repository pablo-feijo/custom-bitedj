#include "widget/weffectchainpresetselector.h"

#include <QAbstractItemView>
#include <QDialog>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <array>
#include <algorithm>
#include <QStyleOption>
#include <QStylePainter>

#include "effects/chains/quickeffectchain.h"
#include "effects/effectsmanager.h"
#include "effects/presets/effectchainpreset.h"
#include "effects/presets/effectpreset.h"
#include "moc_weffectchainpresetselector.cpp"
#include "widget/effectwidgetutils.h"
#include "skin/highcontrast.h"

class QPaintEvent;

WEffectChainPresetSelector::WEffectChainPresetSelector(
        QWidget* pParent, EffectsManager* pEffectsManager)
        : QComboBox(pParent),
          WBaseWidget(this),
          m_bQuickEffectChain(false),
          m_pChainPresetManager(pEffectsManager->getChainPresetManager()),
          m_pEffectsManager(pEffectsManager) {
    // Prevent this widget from getting focused by Tab/Shift+Tab
    // to avoid interfering with using the library via keyboard.
    // Allow click focus though so the list can always be opened by mouse,
    // see https://github.com/mixxxdj/mixxx/issues/10184
    setFocusPolicy(Qt::ClickFocus);
}

void WEffectChainPresetSelector::setup(const QDomNode& node, const SkinContext& context) {
    m_pChain = EffectWidgetUtils::getEffectChainFromNode(
            node, context, m_pEffectsManager);

    VERIFY_OR_DEBUG_ASSERT(m_pChain != nullptr) {
        SKIN_WARNING(node,
                context,
                QStringLiteral("EffectChainPresetSelector node could not "
                               "attach to EffectChain"));
        return;
    }

    auto* pQuickEffectChain = qobject_cast<QuickEffectChain*>(m_pChain.data());
    if (pQuickEffectChain) {
        connect(m_pChainPresetManager.data(),
                &EffectChainPresetManager::quickEffectChainPresetListUpdated,
                this,
                &WEffectChainPresetSelector::populate);
        m_bQuickEffectChain = true;
    } else {
        connect(m_pChainPresetManager.data(),
                &EffectChainPresetManager::effectChainPresetListUpdated,
                this,
                &WEffectChainPresetSelector::populate);
    }
    connect(m_pChain.data(),
            &EffectChain::chainPresetChanged,
            this,
            &WEffectChainPresetSelector::slotChainPresetChanged);
    connect(this,
            QOverload<int>::of(&QComboBox::activated),
            this,
            &WEffectChainPresetSelector::slotEffectChainPresetSelected);

    populate();
}

void WEffectChainPresetSelector::populate() {
    blockSignals(true);
    clear();

    QList<EffectChainPresetPointer> presetList;
    if (m_bQuickEffectChain) {
        presetList = m_pEffectsManager->getChainPresetManager()->getQuickEffectPresetsSorted();
    } else {
        presetList = m_pEffectsManager->getChainPresetManager()->getPresetsSorted();
    }

    const EffectsBackendManagerPointer pBackendManager = m_pEffectsManager->getBackendManager();
    QStringList effectNames;
    for (int i = 0; i < presetList.size(); i++) {
        auto pChainPreset = presetList.at(i);
        QString elidedDisplayName = pChainPreset->displayName();
        addItem(elidedDisplayName, QVariant(pChainPreset->name()));
        QString tooltip =
                QStringLiteral("<b>") + pChainPreset->displayName().toHtmlEscaped() + QStringLiteral("</b>");
        if (!pChainPreset->description().isEmpty()) {
            tooltip += QStringLiteral("<br/>") + pChainPreset->description().toHtmlEscaped();
        }
        for (const auto& pEffectPreset : pChainPreset->effectPresets()) {
            if (!pEffectPreset->isEmpty()) {
                EffectManifestPointer pManifest = pBackendManager->getManifest(pEffectPreset);
                if (pManifest) {
                    effectNames.append(pManifest->name());
                }
            }
        }
        if (effectNames.size() > 1) {
            tooltip.append("<br/>");
            tooltip.append(effectNames.join("<br/>"));
        }
        effectNames.clear();
        setItemData(i, tooltip, Qt::ToolTipRole);
    }

    slotChainPresetChanged(m_pChain->presetName());
    blockSignals(false);
}

void WEffectChainPresetSelector::showPopup() {
    if (m_bQuickEffectChain || !m_pChain) {
        QComboBox::showPopup();
        return;
    }

    // A fullscreen layout is intentional on the appliance's Wayland compositor.
    // Paging avoids small scrolling targets and never changes the loaded effect.
    QDialog picker(this, Qt::Dialog | Qt::FramelessWindowHint);
    picker.setObjectName(QStringLiteral("BeatFxPicker"));
    picker.setWindowTitle(tr("Beat FX"));
    picker.setStyleSheet(HighContrast::mapStyleSheet(QStringLiteral(
            "QDialog#BeatFxPicker { background: #111111; color: #dddddd; }"
            "QDialog#BeatFxPicker QLabel { color: #dddddd; }"
            "QDialog#BeatFxPicker QPushButton { font-size: 16px; padding: 4px 12px;"
            "border: 1px solid #444444; border-radius: 6px;"
            "background: #1a1a1a; color: #dddddd; }"
            "QDialog#BeatFxPicker QPushButton:checked { border: 2px solid #835aa0;"
            "background: #835aa0; color: #ffffff; }"
            "QDialog#BeatFxPicker QPushButton:focus { border: 2px solid #835aa0; }"
            "QDialog#BeatFxPicker QPushButton:disabled { color: #666666; }")));
    auto* layout = new QVBoxLayout(&picker);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(8);
    auto* header = new QHBoxLayout();
    auto* title = new QLabel(tr("BEAT FX"), &picker);
    header->addWidget(title, 1);
    auto* standard = new QPushButton(tr("Standard"), &picker);
    auto* saved = new QPushButton(tr("Saved"), &picker);
    auto* clear = new QPushButton(tr("Clear FX"), &picker);
    auto* close = new QPushButton(tr("Close"), &picker);
    for (auto* button : {standard, saved, clear, close}) {
        button->setMinimumSize(120, 48);
        button->setAutoDefault(false);
        header->addWidget(button);
    }
    standard->setCheckable(true);
    saved->setCheckable(true);
    layout->addLayout(header);
    auto* note = new QLabel(tr("Native approximations • Select an effect, then use FX ON to activate."), &picker);
    note->setMinimumHeight(24);
    layout->addWidget(note);
    auto* grid = new QGridLayout();
    grid->setSpacing(8);
    std::array<QPushButton*, 14> buttons;
    for (int i = 0; i < 14; ++i) {
        auto* button = new QPushButton(&picker);
        button->setMinimumHeight(50);
        QSizePolicy policy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        policy.setRetainSizeWhenHidden(true);
        button->setSizePolicy(policy);
        button->setCheckable(true);
        button->setAutoDefault(false);
        buttons[i] = button;
        grid->addWidget(button, i / 2, i % 2);
    }
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);
    layout->addLayout(grid, 1);
    auto* navigation = new QHBoxLayout();
    auto* previous = new QPushButton(tr("Previous"), &picker);
    auto* next = new QPushButton(tr("Next"), &picker);
    auto* pageLabel = new QLabel(&picker);
    pageLabel->setAlignment(Qt::AlignCenter);
    for (auto* button : {previous, next}) {
        button->setMinimumSize(180, 48);
        button->setAutoDefault(false);
    }
    navigation->addWidget(previous);
    navigation->addWidget(pageLabel, 1);
    navigation->addWidget(next);
    layout->addLayout(navigation);

    QList<int> standardItems, savedItems;
    for (int i = 0; i < count(); ++i) {
        auto preset = m_pChainPresetManager->getPreset(itemData(i).toString());
        if (!preset || preset->isEmpty()) {
            continue;
        }
        (preset->isRekordbox7() ? standardItems : savedItems).append(i);
    }
    bool showSaved = savedItems.contains(currentIndex());
    int page = std::max(0, static_cast<int>((showSaved ? savedItems : standardItems).indexOf(currentIndex()))) / 14;
    const auto refresh = [&]() {
        const auto& items = showSaved ? savedItems : standardItems;
        int pages = std::max(1, (static_cast<int>(items.size()) + 13) / 14);
        page = std::clamp(page, 0, pages - 1);
        standard->setChecked(!showSaved);
        saved->setChecked(showSaved);
        note->setText(showSaved ? tr("Saved and legacy presets • Original files and settings are preserved.") :
                tr("Native approximations • Select an effect, then use FX ON to activate."));
        pageLabel->setText(tr("Page %1 of %2").arg(page + 1).arg(pages));
        previous->setEnabled(page > 0);
        next->setEnabled(page + 1 < pages);
        for (int i = 0; i < 14; ++i) {
            int offset = page * 14 + i;
            auto* button = buttons[i];
            int index = offset < items.size() ? items[offset] : -1;
            button->setProperty("presetIndex", index);
            button->setEnabled(index >= 0);
            button->setVisible(index >= 0);
            button->setText(index >= 0 ? QStringLiteral("%1   %2").arg(offset + 1, 2, 10, QLatin1Char('0')).arg(itemText(index)) : QString());
            button->setChecked(index >= 0 && index == currentIndex());
            button->setToolTip(index >= 0 ? itemData(index, Qt::ToolTipRole).toString() : QString());
        }
    };
    for (auto* button : buttons) {
        connect(button, &QPushButton::clicked, &picker, [&, button]() {
            int index = button->property("presetIndex").toInt();
            if (index >= 0) {
                setCurrentIndex(index);
                slotEffectChainPresetSelected(index);
                picker.accept();
            }
        });
    }
    connect(previous, &QPushButton::clicked, &picker, [&]() { --page; refresh(); });
    connect(next, &QPushButton::clicked, &picker, [&]() { ++page; refresh(); });
    connect(standard, &QPushButton::clicked, &picker, [&]() { showSaved = false; page = 0; refresh(); });
    connect(saved, &QPushButton::clicked, &picker, [&]() { showSaved = true; page = 0; refresh(); });
    connect(close, &QPushButton::clicked, &picker, &QDialog::reject);
    connect(clear, &QPushButton::clicked, &picker, [&]() {
        setCurrentIndex(findData(kNoEffectString));
        slotEffectChainPresetSelected(currentIndex());
        picker.accept();
    });
    connect(m_pChain.data(), &EffectChain::chainPresetChanged, &picker, [&]() { refresh(); });
    refresh();
    picker.showFullScreen();
    picker.exec();
    // Reset QComboBox's popup state, including keyboard activation.
    QComboBox::hidePopup();
}

void WEffectChainPresetSelector::slotEffectChainPresetSelected(int index) {
    Q_UNUSED(index);
    m_pChain->loadChainPreset(
            m_pChainPresetManager->getPreset(currentData().toString()));
    // Clicking a chain item moves keyboard focus to the list view.
    // Move focus back to the previously focused library widget.
    ControlObject::set(ConfigKey("[Library]", "refocus_prev_widget"), 1);
}

void WEffectChainPresetSelector::slotChainPresetChanged(const QString& name) {
    setCurrentIndex(findData(name));
    setBaseTooltip(itemData(currentIndex(), Qt::ToolTipRole).toString());
}

bool WEffectChainPresetSelector::event(QEvent* pEvent) {
    if (pEvent->type() == QEvent::ToolTip) {
        updateTooltip();
    } else if (pEvent->type() == QEvent::Wheel && !hasFocus()) {
        // don't change preset by scrolling hovered preset selector
        return true;
    }

    return QComboBox::event(pEvent);
}

void WEffectChainPresetSelector::paintEvent(QPaintEvent* e) {
    Q_UNUSED(e);
    // The default paint implementation aligns the text based on the layout direction.
    // Override to allow qss to align the text of the closed combobox with the
    // Quick effect controls in the mixer.
    QStylePainter painter(this);
    QStyleOptionComboBox comboStyle;
    // Initialize the style and draw the frame, down-arrow etc.
    // Note: using 'comboStyle.initFrom(this)' and 'painter.drawComplexControl(...)
    // here would not paint the hover style of the down arrow.
    initStyleOption(&comboStyle);
    style()->drawComplexControl(QStyle::CC_ComboBox, &comboStyle, &painter, this);

    QStyleOptionButton buttonStyle;
    buttonStyle.initFrom(this);
    QRect buttonRect = style()->subControlRect(
            QStyle::CC_ComboBox, &comboStyle, QStyle::SC_ComboBoxEditField, this);
    buttonStyle.rect = buttonRect;
    QFontMetrics metrics(font());
    // Since the chain selector and the popup can differ in width,
    // elide the button text independently from the popup display name.
    buttonStyle.text = metrics.elidedText(
            currentText(),
            Qt::ElideRight,
            buttonRect.width());
    // Draw the text for the selector button. Alternative: painter.drawControl(...)
    style()->drawControl(QStyle::CE_PushButtonLabel, &buttonStyle, &painter, this);
}
