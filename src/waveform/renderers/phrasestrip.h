#pragma once

#include <QPainter>
#include "control/controlobject.h"
#include <algorithm>
#include <cmath>
#include "track/phrase.h"
#include "util/fpclassify.h"

namespace mixxx {

inline bool showWaveformPhrases() {
    auto* control = ControlObject::getControl(
            ConfigKey("[BiteDJ]", "show_phrases"), ControlFlag::NoWarnIfMissing);
    return !control || control->toBool();
}


inline QColor phraseColor(Phrase::Kind kind) {
    switch (kind) {
    case Phrase::Kind::Intro: return QColor(215, 30, 35);
    case Phrase::Kind::Verse: return QColor(48, 113, 73);
    case Phrase::Kind::Bridge: return QColor(121, 75, 145);
    case Phrase::Kind::Chorus: return QColor(24, 112, 16);
    case Phrase::Kind::Up: return QColor(145, 48, 220);
    case Phrase::Kind::Down: return QColor(110, 86, 30);
    case Phrase::Kind::Outro: return QColor(47, 92, 137);
    default: return QColor(80, 80, 80);
    }
}

// Non-interactive overlay: never owns mouse events or cue slots.
inline void paintPhraseStrip(QPainter& painter, const PhraseList& phrases,
        QRectF bounds, Qt::Orientation orientation, double first, double last,
        double scale = 1, bool overviewRow = false) {
    if (!showWaveformPhrases() || phrases.isEmpty() || bounds.isEmpty() || !util_isfinite(first) ||
            !util_isfinite(last) || last <= first || !util_isfinite(scale) || scale <= 0) return;
    painter.save();
    painter.setClipRect(bounds, Qt::IntersectClip);
    painter.translate(bounds.topLeft());
    double length = bounds.width(), breadth = bounds.height();
    if (orientation == Qt::Vertical) {
        painter.translate(bounds.width(), 0);
        painter.rotate(90);
        std::swap(length, breadth);
    }
    const double stripHeight = overviewRow ? breadth : std::min(10 * scale, breadth / 3);
    const double y = breadth - stripHeight;
    QFont font = painter.font();
    font.setPixelSize(std::max(1, int(stripHeight - 2 * scale)));
    painter.setFont(font);
    const auto xAt = [&](double seconds) {
        return std::clamp((seconds - first) / (last - first), 0.0, 1.0) * length;
    };
    auto firstPhrase = std::lower_bound(phrases.cbegin(), phrases.cend(), first,
            [](const auto& phrase, double time) { return phrase.endSeconds <= time; });
    for (auto it = firstPhrase; it != phrases.cend() && it->startSeconds < last; ++it) {
        const auto& phrase = *it;
        if (phrase.endSeconds <= first || phrase.startSeconds >= last) continue;
        const double start = xAt(phrase.startSeconds), end = xAt(phrase.endSeconds);
        const QRectF block(start, y, end - start, stripHeight);
        QColor color = phraseColor(phrase.kind);
        // Opaque dark colors keep white labels readable in both Day and Night.
        painter.fillRect(block, color);
        if (!overviewRow && phrase.fillSeconds >= phrase.startSeconds && phrase.fillSeconds < phrase.endSeconds) {
            painter.fillRect(QRectF(xAt(phrase.fillSeconds), y,
                                     end - xAt(phrase.fillSeconds), stripHeight),
                    QBrush(QColor(255, 255, 255, 100), Qt::BDiagPattern));
        }
        painter.setPen(QColor(20, 20, 20));
        painter.drawLine(QPointF(start, y), QPointF(start, breadth));
        if (stripHeight >= 10 && painter.fontMetrics().horizontalAdvance(phrase.label) + 6 * scale < block.width()) {
            painter.setPen(Qt::white);
            painter.drawText(block.adjusted(3 * scale, 0, -3 * scale, 0),
                    Qt::AlignLeft | Qt::AlignVCenter, phrase.label);
        }
    }
    painter.restore();
}

} // namespace mixxx
