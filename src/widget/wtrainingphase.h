#pragma once

#include <array>
#include <memory>

#include <QColor>
#include <QSharedPointer>

#include "track/beats.h"
#include "track/track_decl.h"
#include "widget/wwidget.h"

class BaseTrackPlayer;
class ControlProxy;
class PlayerManager;
class QDomNode;
class SkinContext;
class VisualPlayPosition;
class VSyncThread;

class WTrainingPhase final : public WWidget {
    Q_OBJECT
  public:
    struct Position {
        bool valid = false;
        int bar = 0;
        int beat = 0;
        double cycle = 0.0;
    };

    explicit WTrainingPhase(PlayerManager* pPlayerManager, QWidget* pParent = nullptr);

    void setup(const QDomNode& node, const SkinContext& context);

    void render(VSyncThread* vsyncThread);

    static Position calculatePosition(const mixxx::BeatsPointer& beats,
            double trackSamples,
            double playPosition);

  protected:
    void paintEvent(QPaintEvent* pEvent) override;

  private:
    friend class DeckPresentationTest;
    static constexpr qreal kSideLabelWidth = 120.0;
    static constexpr qreal kMeterGap = 8.0;
    struct DeckState {
        TrackPointer track;
        BaseTrackPlayer* player = nullptr;
        ControlProxy* playPosition = nullptr;
        ControlProxy* trackSamples = nullptr;
        QSharedPointer<VisualPlayPosition> visualPlayPosition;
        double visualPosition = -1.0;
    };

    Position deckPosition(int index) const;
    static QString positionText(const Position& position);
    void drawBoxes(QPainter* painter, const QRectF& area, const Position& position,
            const QColor& color) const;
    void drawLine(QPainter* painter, const QRectF& area, const Position& position,
            const QColor& color, bool upperDeck) const;
    void drawDeck(QPainter* painter, int index, const QRectF& row, int style) const;

    std::array<DeckState, 2> m_decks;
    ControlProxy* m_pStyle;
    QColor m_deckColors[2]{QColor("#ff4a45"), QColor("#12b9ff")};
    QColor m_lineColors[2]{QColor("#d79a3b"), QColor("#12b9ff")};
    QColor m_activeColor{QColor("#ff9f2d")};
    QColor m_lineActiveColor{QColor("#ff4a45")};
    QColor m_markerColor{Qt::white};
};
