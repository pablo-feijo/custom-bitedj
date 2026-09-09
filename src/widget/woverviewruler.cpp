#include "widget/woverviewruler.h"
#include <QPainter>
#include <cmath>
#include "mixer/playerinfo.h"
#include "skin/legacy/skincontext.h"
#include "track/track.h"
#include "widget/wskincolor.h"
#include "moc_woverviewruler.cpp"

WOverviewRuler::WOverviewRuler(QWidget* parent) : WWidget(parent) {
    connect(&m_timer, &QTimer::timeout, this, [this] { if (isVisible()) update(); });
    m_timer.start(1000);
}
void WOverviewRuler::setup(const QDomNode& node, const SkinContext& context) {
    m_group = QStringLiteral("[Channel%1]").arg(context.selectString(node, "Channel"));
}
void WOverviewRuler::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.fillRect(rect(), WSkinColor::getCorrectColor(QColor("#101018")));
    const auto track = PlayerInfo::instance().getTrackInfo(m_group);
    if (!track || track->getDuration() <= 0 || width() < 60) return;
    const double duration = track->getDuration();
    const int step = 60 * qMax(1, static_cast<int>(std::ceil(duration / 60.0 / qMax(1, width() / 60))));
    QFont f = font(); f.setPixelSize(6); painter.setFont(f);
    painter.setPen(WSkinColor::getCorrectColor(QColor("#aaaaba")));
    for (int seconds = 0; seconds <= duration; seconds += step) {
        const int x = qRound(seconds / duration * (width() - 1));
        painter.drawLine(x, 0, x, 1);
        const QString label = QStringLiteral("%1:00").arg(seconds / 60);
        const int labelWidth = painter.fontMetrics().horizontalAdvance(label);
        const int left = qBound(4, x - labelWidth / 2, width() - labelWidth - 4);
        painter.drawText(left, height() - 1, label);
    }
}
