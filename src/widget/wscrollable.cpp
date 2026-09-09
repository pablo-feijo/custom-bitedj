#include "widget/wscrollable.h"

#include <QScroller>
#include <QScrollBar>
#include <QTimer>
#include "control/controlproxy.h"
#include "moc_wscrollable.cpp"
#include "skin/legacy/skincontext.h"

WScrollable::WScrollable(QWidget* pParent)
        : QScrollArea(pParent),
          WBaseWidget(this) {
}

void WScrollable::setup(const QDomNode& node, const SkinContext& context) {
    if (context.selectString(node, "TouchScroll") == QStringLiteral("true")) {
        QScroller::grabGesture(viewport(), QScroller::TouchGesture);
    }
    const auto resetGroup = context.selectString(node, "ResetOnEffectChange");
    if (!resetGroup.isEmpty()) {
        auto* loaded = new ControlProxy(resetGroup, "loaded", this);
        loaded->connectValueChanged(this, [this](double value) {
            if (value > 0) {
                QTimer::singleShot(0, this, [this] { verticalScrollBar()->setValue(0); });
            }
        });
    }
    QString horizontalPolicy;
    // The QT default is "As Needed", so we don't need a selector for that.
    if (context.hasNodeSelectString(node, "HorizontalScrollBarPolicy", &horizontalPolicy)) {
        if (horizontalPolicy == "on") {
            setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        } else if (horizontalPolicy == "off") {
            setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }
    }
    QString verticalPolicy;
    if (context.hasNodeSelectString(node, "VerticalScrollBarPolicy", &verticalPolicy)) {
        if (verticalPolicy == "on") {
            setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        } else if (verticalPolicy == "off") {
            setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }
    }
}
