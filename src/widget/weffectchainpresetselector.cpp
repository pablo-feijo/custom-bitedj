#include "widget/weffectchainpresetselector.h"

#include <QAbstractItemView>
#include <QDialog>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QPainter>
#include <QPolygonF>
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

namespace {
enum class PickerAction { Clear, Close, Previous, Next };

void setPickerAction(QPushButton* button, PickerAction action) {
    button->setAccessibleName(button->text());
    button->setToolTip(button->text());
    button->setText(QString());
    constexpr int size = 20;
    const qreal scale = button->devicePixelRatioF();
    QPixmap pixmap(qRound(size * scale), qRound(size * scale));
    pixmap.setDevicePixelRatio(scale);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(HighContrast::mapColor(QColor("#dddddd")),
            1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    switch (action) {
    case PickerAction::Clear:
        // Eraser: unload the current chain, without deleting its preset.
        painter.drawPolygon(QPolygonF{QPointF(3, 11), QPointF(11, 3),
                QPointF(17, 9), QPointF(9, 17), QPointF(6, 17)});
        painter.drawLine(QPointF(6, 8), QPointF(12, 14));
        painter.drawLine(QPointF(9, 17), QPointF(18, 17));
        break;
    case PickerAction::Close:
        painter.drawLine(QPointF(5, 5), QPointF(15, 15));
        painter.drawLine(QPointF(15, 5), QPointF(5, 15));
        break;
    case PickerAction::Previous:
        painter.drawPolyline(QPolygonF{QPointF(13, 4), QPointF(7, 10), QPointF(13, 16)});
        break;
    case PickerAction::Next:
        painter.drawPolyline(QPolygonF{QPointF(7, 4), QPointF(13, 10), QPointF(7, 16)});
        break;
    }
    painter.end();
    button->setIcon(QIcon(pixmap));
    button->setIconSize(QSize(size, size));
}

class PanelEffectPicker : public QDialog {
  public:
    explicit PanelEffectPicker(QWidget* parent) : QDialog(parent) {
        setWindowFlags(Qt::Widget);
        setAttribute(Qt::WA_StyledBackground);
    }

  protected:
    bool eventFilter(QObject* watched, QEvent* event) override {
        // Switching pages or reloading the skin dismisses the child picker.
        if (event->type() == QEvent::Hide) {
            reject();
        }
        return QDialog::eventFilter(watched, event);
    }
};
} // namespace


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

    // Embed the picker in the skin so Wayland cannot tile it as another window.
    // Its bounds follow the selector and leave the decks visible.
    PanelEffectPicker picker(window());
    installEventFilter(&picker);
    picker.setObjectName(QStringLiteral("BeatFxPicker"));
    picker.setWindowTitle(tr("Beat FX"));
    picker.setStyleSheet(HighContrast::mapStyleSheet(QStringLiteral(
            "QDialog#BeatFxPicker { background: #111111; color: #dddddd; }"
            "QDialog#BeatFxPicker QLabel { color: #dddddd; font-size: 9px; }"
            "QDialog#BeatFxPicker QPushButton { font-size: 10px; padding: 2px;"
            "border: 1px solid #444444; border-radius: 6px;"
            "background: #1a1a1a; color: #dddddd; }"
            "QDialog#BeatFxPicker QPushButton:checked { border: 2px solid #835aa0;"
            "background: #835aa0; color: #ffffff; }"
            "QDialog#BeatFxPicker QPushButton:focus { border: 2px solid #835aa0; }"
            "QDialog#BeatFxPicker QPushButton:disabled { color: #666666; }")));
    auto* layout = new QVBoxLayout(&picker);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);
    auto* header = new QGridLayout();
    header->setSpacing(4);
    auto* standard = new QPushButton(tr("Standard"), &picker);
    auto* saved = new QPushButton(tr("Saved"), &picker);
    auto* clear = new QPushButton(tr("Clear FX"), &picker);
    auto* close = new QPushButton(tr("Close"), &picker);
    for (auto* button : {standard, saved, clear, close}) {
        button->setMinimumSize(0, 30);
        button->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        button->setAutoDefault(false);
    }
    header->addWidget(standard, 0, 0);
    header->addWidget(saved, 0, 1);
    header->addWidget(clear, 1, 0);
    header->addWidget(close, 1, 1);
    setPickerAction(clear, PickerAction::Clear);
    setPickerAction(close, PickerAction::Close);
    standard->setCheckable(true);
    saved->setCheckable(true);
    layout->addLayout(header);
    auto* grid = new QGridLayout();
    grid->setSpacing(4);
    grid->setAlignment(Qt::AlignTop);
    constexpr int kPageSize = 14;
    std::array<QPushButton*, kPageSize> buttons;
    for (int i = 0; i < kPageSize; ++i) {
        auto* button = new QPushButton(&picker);
        button->setFixedHeight(44);
        QSizePolicy policy(QSizePolicy::Ignored, QSizePolicy::Expanding);
        policy.setRetainSizeWhenHidden(true);
        button->setSizePolicy(policy);
        button->setCheckable(true);
        button->setAutoDefault(false);
        buttons[i] = button;
        grid->addWidget(button, i / 2, i % 2);
    }
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);
    layout->addLayout(grid);
    auto* navigation = new QHBoxLayout();
    auto* previous = new QPushButton(tr("Prev"), &picker);
    auto* next = new QPushButton(tr("Next"), &picker);
    auto* pageLabel = new QLabel(&picker);
    pageLabel->setAlignment(Qt::AlignCenter);
    for (auto* button : {previous, next}) {
        button->setMinimumSize(0, 30);
        button->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        button->setAutoDefault(false);
    }
    setPickerAction(previous, PickerAction::Previous);
    setPickerAction(next, PickerAction::Next);
    navigation->addWidget(previous);
    navigation->addWidget(next);
    layout->addWidget(pageLabel);
    layout->addLayout(navigation);
    layout->addStretch(1);

    QList<int> standardItems, savedItems;
    for (int i = 0; i < count(); ++i) {
        auto preset = m_pChainPresetManager->getPreset(itemData(i).toString());
        if (!preset || preset->isEmpty()) {
            continue;
        }
        (preset->isRekordbox7() ? standardItems : savedItems).append(i);
    }
    bool showSaved = savedItems.contains(currentIndex());
    int page = std::max(0, static_cast<int>((showSaved ? savedItems : standardItems).indexOf(currentIndex()))) / kPageSize;
    const auto refresh = [&]() {
        const auto& items = showSaved ? savedItems : standardItems;
        int pages = std::max(1, (static_cast<int>(items.size()) + kPageSize - 1) / kPageSize);
        page = std::clamp(page, 0, pages - 1);
        standard->setChecked(!showSaved);
        saved->setChecked(showSaved);
        pageLabel->setText(tr("%1 / %2").arg(page + 1).arg(pages));
        pageLabel->setAccessibleName(tr("Page %1 of %2").arg(page + 1).arg(pages));
        previous->setEnabled(page > 0);
        next->setEnabled(page + 1 < pages);
        for (int i = 0; i < kPageSize; ++i) {
            int offset = page * kPageSize + i;
            auto* button = buttons[i];
            int index = offset < items.size() ? items[offset] : -1;
            button->setProperty("presetIndex", index);
            button->setEnabled(index >= 0);
            button->setVisible(index >= 0);
            QString label = index >= 0 ? itemText(index) : QString();
            // Wrap against the actual cell width, including Saved names with
            // number prefixes. Long custom words remain identifiable by tooltip.
            button->ensurePolished();
            const QFontMetrics metrics(button->font());
            const int textWidth = (width() - 12) / 2 - 8;
            QStringList lines;
            QString line;
            for (const auto& word : label.split(QLatin1Char(' '))) {
                const QString candidate = line.isEmpty() ? word : line + QLatin1Char(' ') + word;
                if (!line.isEmpty() && (candidate.size() > 10 || metrics.horizontalAdvance(candidate) > textWidth - 2)) {
                    lines.append(line);
                    line = word;
                } else {
                    line = candidate;
                }
            }
            lines.append(line);
            for (auto& text : lines) {
                text = metrics.elidedText(text, Qt::ElideRight, textWidth);
            }
            label = lines.join(QLatin1Char('\n'));
            button->setText(label);
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
    const QPoint origin = mapTo(window(), QPoint(0, 0));
    picker.setGeometry(origin.x(), origin.y(), width(), window()->height() - origin.y());
    picker.raise();
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
