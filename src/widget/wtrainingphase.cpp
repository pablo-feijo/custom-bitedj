#include "widget/wtrainingphase.h"

#include <algorithm>
#include <cmath>

#include <QPainter>
#include <QPaintEvent>

#include "control/controlproxy.h"
#include "mixer/basetrackplayer.h"
#include "mixer/playermanager.h"
#include "moc_wtrainingphase.cpp"
#include "skin/legacy/skincontext.h"
#include "track/beats.h"
#include "track/track.h"
#include "widget/wskincolor.h"
#include "waveform/visualplayposition.h"

namespace {
constexpr int kBeatsPerBar = 4;
} // namespace

WTrainingPhase::WTrainingPhase(PlayerManager* pPlayerManager, QWidget* pParent)
        : WWidget(pParent),
          m_pStyle(new ControlProxy(
                  "[BiteDJ]", "training_mode", this, ControlFlag::NoAssertIfMissing)) {
    m_pStyle->connectValueChanged(this, [this](double) { update(); });

    const std::array<QString, 2> groups{
            QStringLiteral("[Channel1]"), QStringLiteral("[Channel2]")};
    for (int i = 0; i < static_cast<int>(m_decks.size()); ++i) {
        auto& deck = m_decks[i];
        deck.player = pPlayerManager ? pPlayerManager->getPlayer(groups[i]) : nullptr;
        deck.playPosition = new ControlProxy(
                groups[i], "playposition", this, ControlFlag::NoAssertIfMissing);
        deck.trackSamples = new ControlProxy(
                groups[i], "track_samples", this, ControlFlag::NoAssertIfMissing);
        deck.visualPlayPosition = VisualPlayPosition::getVisualPlayPosition(groups[i]);
        deck.trackSamples->connectValueChanged(this, [this](double) { update(); });
        if (deck.player) {
            deck.track = deck.player->getLoadedTrack();
            connect(deck.player,
                    &BaseTrackPlayer::loadingTrack,
                    this,
                    [this, i](TrackPointer, TrackPointer) {
                        m_decks[i].track.reset();
                        m_decks[i].visualPosition = -1.0;
                        update();
                    });
            connect(deck.player,
                    &BaseTrackPlayer::newTrackLoaded,
                    this,
                    [this, i](TrackPointer track) {
                        m_decks[i].track = std::move(track);
                        update();
                    });
        }
    }
}

void WTrainingPhase::setup(const QDomNode& node, const SkinContext& context) {
    const auto readColor = [&context, &node](const QString& name, QColor fallback) {
        const QColor color(context.selectString(node, name));
        return WSkinColor::getCorrectColor(color.isValid() ? color : fallback);
    };
    m_deckColors[0] = readColor(QStringLiteral("Deck1Color"), m_deckColors[0]);
    m_deckColors[1] = readColor(QStringLiteral("Deck2Color"), m_deckColors[1]);
    m_lineColors[0] = readColor(QStringLiteral("LineDeck1Color"), m_lineColors[0]);
    m_lineColors[1] = readColor(QStringLiteral("LineDeck2Color"), m_lineColors[1]);
    m_activeColor = readColor(QStringLiteral("ActiveColor"), m_activeColor);
    m_lineActiveColor =
            readColor(QStringLiteral("LineActiveColor"), m_lineActiveColor);
    m_markerColor = readColor(QStringLiteral("MarkerColor"), m_markerColor);
}

WTrainingPhase::Position WTrainingPhase::calculatePosition(
        const mixxx::BeatsPointer& beats,
        double trackSamples,
        double playPosition) {
    Position result;
    if (!beats || !std::isfinite(trackSamples) || trackSamples <= 0.0 ||
            !std::isfinite(playPosition) || playPosition < 0.0) {
        return result;
    }

    const auto framePosition = mixxx::audio::FramePos::fromEngineSamplePos(
            std::clamp(playPosition, 0.0, 1.0) * trackSamples);
    auto beatIt = beats->iteratorFrom(framePosition);
    if (*beatIt > framePosition) {
        --beatIt;
    }
    const int beatIndex = std::max(0, beatIt - beats->cfirstmarker());
    const auto nextBeatIt = beatIt + 1;
    const double beatLength = *nextBeatIt - *beatIt;
    if (beatLength <= 0.0) {
        return result;
    }
    // Derive the fractional beat from the same continuously updated play
    // position used to choose the beat. The engine's separate beat-distance
    // control can arrive on a different update and made thin grid markers
    // visibly step or jump between frames.
    const double distance = std::clamp(
            static_cast<double>(framePosition - *beatIt) / beatLength, 0.0, 1.0);
    result.valid = true;
    result.bar = beatIndex / kBeatsPerBar + 1;
    result.beat = beatIndex % kBeatsPerBar + 1;
    result.cycle = (beatIndex % kBeatsPerBar + distance) / kBeatsPerBar;
    return result;
}

void WTrainingPhase::render(VSyncThread* vsyncThread) {
    if (!isVisible() || static_cast<int>(m_pStyle->get()) == 0) {
        return;
    }
    for (auto& deck : m_decks) {
        // Use one audio-clock snapshot for both the whole beat and its fraction.
        // The waveform interpolator also handles rate, reverse and loop wraps.
        deck.visualPosition = vsyncThread
                ? deck.visualPlayPosition->getAtNextVSync(vsyncThread)
                : deck.visualPlayPosition->getEnginePlayPos();
    }
    // Complete the QWidget paint within the shared render tick. A queued
    // update can be coalesced with the next tick when the GUI is busy, leaving
    // the previous phase on screen for another frame. Limit background damage
    // to the phase rows (including the reference line and bar labels).
    repaint(0, height() / 2 - 80, width(), 160);
}

WTrainingPhase::Position WTrainingPhase::deckPosition(int index) const {
    const auto& deck = m_decks[index];
    return calculatePosition(deck.track ? deck.track->getBeats() : nullptr,
            deck.trackSamples->get(),
            deck.visualPosition >= 0.0
                    ? deck.visualPosition
                    : deck.playPosition->get());
}

void WTrainingPhase::paintEvent(QPaintEvent* pEvent) {
    Q_UNUSED(pEvent);
    const int style = static_cast<int>(m_pStyle->get());
    if (style < 1 || style > 2) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const qreal contentWidth = std::min<qreal>(width() - 32.0, 900.0);
    const qreal left = (width() - contentWidth) / 2.0;
    const qreal centerY = height() / 2.0;

    const QRectF firstRow = style == 1
            ? QRectF(left, centerY - 36.0, contentWidth, 32.0)
            : QRectF(left, centerY - 55.0, contentWidth, 46.0);
    const QRectF secondRow = style == 1
            ? QRectF(left, centerY + 4.0, contentWidth, 32.0)
            : QRectF(left, centerY + 17.0, contentWidth, 46.0);
    drawDeck(&painter, 0, firstRow, style);
    drawDeck(&painter, 1, secondRow, style);

    const qreal meterInset = kSideLabelWidth + kMeterGap;
    const qreal meterLeft = left + meterInset;
    const qreal meterRight = left + contentWidth - meterInset;
    painter.setPen(QPen(m_markerColor, 2.0));
    painter.drawLine(QPointF((meterLeft + meterRight) / 2.0, firstRow.top() - 14.0),
            QPointF((meterLeft + meterRight) / 2.0, secondRow.bottom() + 14.0));
}

void WTrainingPhase::drawDeck(
        QPainter* painter, int index, const QRectF& row, int style) const {
    QFont labelFont = painter->font();
    labelFont.setBold(true);
    labelFont.setPixelSize(17);
    painter->setFont(labelFont);
    painter->setPen(m_deckColors[index]);
    painter->drawText(QRectF(row.left(), row.top(), kSideLabelWidth, row.height()),
            Qt::AlignLeft | Qt::AlignVCenter,
            tr("DECK %1").arg(index + 1));

    const qreal meterInset = kSideLabelWidth + kMeterGap;
    const QRectF meter(row.left() + meterInset,
            row.top() + 2.0,
            std::max(80.0, row.width() - 2.0 * meterInset),
            row.height() - 4.0);
    const Position position = deckPosition(index);
    if (style == 1) {
        drawLine(painter, meter, position, m_lineColors[index], index == 0);
    } else {
        drawBoxes(painter, meter, position, m_deckColors[index]);
    }

    painter->setPen(m_deckColors[index]);
    const QString text = positionText(position);
    painter->drawText(QRectF(row.right() - kSideLabelWidth,
                              row.top(),
                              kSideLabelWidth,
                              row.height()),
            Qt::AlignRight | Qt::AlignVCenter,
            text);
}

QString WTrainingPhase::positionText(const Position& position) {
    return position.valid
            ? QStringLiteral("%1.%2 BARS")
                      .arg(position.bar, 2, 10, QLatin1Char('0'))
                      .arg(position.beat)
            : QStringLiteral("--.- BARS");
}

void WTrainingPhase::drawBoxes(QPainter* painter,
        const QRectF& area,
        const Position& position,
        const QColor& color) const {
    constexpr qreal gap = 9.0;
    const qreal boxWidth = (area.width() - gap * 3.0) / 4.0;
    const int activeBeat = position.valid
            ? static_cast<int>(std::floor(position.cycle * kBeatsPerBar)) % kBeatsPerBar
            : -1;
    for (int beat = 0; beat < kBeatsPerBar; ++beat) {
        const QRectF box(area.left() + beat * (boxWidth + gap),
                area.top() + 3.0,
                boxWidth,
                area.height() - 6.0);
        if (beat == activeBeat) {
            painter->setPen(QPen(m_activeColor, 2.0));
            painter->setBrush(m_activeColor);
        } else {
            painter->setPen(QPen(color, 2.0));
            painter->setBrush(Qt::NoBrush);
        }
        painter->drawRoundedRect(box, 4.0, 4.0);
    }
}

void WTrainingPhase::drawLine(QPainter* painter,
        const QRectF& area,
        const Position& position,
        const QColor& color,
        bool upperDeck) const {
    const qreal y = area.center().y();
    painter->setPen(QPen(color, 2.0));
    painter->drawLine(QPointF(area.left(), y), QPointF(area.right(), y));
    if (!position.valid) {
        return;
    }

    // Match the legacy CDJ line phase meter: the center playhead is fixed and
    // each deck's beat grid scrolls underneath it. Two bars are visible, with
    // one small marker per beat and a red marker on beat one.
    constexpr int kVisibleBeats = 8;
    const qreal beatSpacing = area.width() / kVisibleBeats;
    const qreal phaseBeat = position.cycle * kBeatsPerBar;
    QColor tickColor = m_markerColor;
    tickColor.setAlpha(150);
    for (int beat = -kVisibleBeats; beat <= kVisibleBeats; ++beat) {
        const qreal x = area.center().x() +
                (beat - phaseBeat) * beatSpacing;
        if (x < area.left() || x > area.right()) {
            continue;
        }
        const bool downbeat =
                ((beat % kBeatsPerBar) + kBeatsPerBar) % kBeatsPerBar == 0;
        if (downbeat) {
            painter->setPen(QPen(m_lineActiveColor, 3.0));
            painter->drawLine(QPointF(x, y - 14.0), QPointF(x, y + 14.0));
            continue;
        }
        constexpr qreal markerHeight = 8.0;
        painter->setPen(QPen(tickColor, 2.0));
        painter->drawLine(QPointF(x, y),
                QPointF(x, y + (upperDeck ? markerHeight : -markerHeight)));
    }
}
