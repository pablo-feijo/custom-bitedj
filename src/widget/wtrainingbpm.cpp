#include "widget/wtrainingbpm.h"

#include <QEvent>
#include <QMouseEvent>

#include "control/controlproxy.h"
#include "moc_wtrainingbpm.cpp"

WTrainingBpm::WTrainingBpm(QWidget* pParent)
        : WNumber(pParent),
          m_pTrainingMode(new ControlProxy(
                  "[BiteDJ]", "training_mode", this, ControlFlag::NoAssertIfMissing)) {
#ifndef __APPLE__
    setAttribute(Qt::WA_AcceptTouchEvents);
#endif
    m_pTrainingMode->connectValueChanged(this, [this](double) {
        if (!m_pTrainingMode->toBool()) {
            m_revealed = false;
        }
        refreshText();
    });
}

void WTrainingBpm::setValue(double value) {
    m_value = value;
    refreshText();
}

bool WTrainingBpm::event(QEvent* pEvent) {
    switch (pEvent->type()) {
    case QEvent::TouchBegin:
        setRevealed(true);
        pEvent->accept();
        return true;
    case QEvent::TouchEnd:
    case QEvent::TouchCancel:
    case QEvent::Hide:
    case QEvent::WindowDeactivate:
        setRevealed(false);
        break;
    default:
        break;
    }
    return WNumber::event(pEvent);
}

void WTrainingBpm::mousePressEvent(QMouseEvent* pEvent) {
    if (pEvent->button() == Qt::LeftButton) {
        setRevealed(true);
        pEvent->accept();
        return;
    }
    WNumber::mousePressEvent(pEvent);
}

void WTrainingBpm::mouseReleaseEvent(QMouseEvent* pEvent) {
    if (pEvent->button() == Qt::LeftButton) {
        setRevealed(false);
        pEvent->accept();
        return;
    }
    WNumber::mouseReleaseEvent(pEvent);
}

void WTrainingBpm::leaveEvent(QEvent* pEvent) {
    setRevealed(false);
    WNumber::leaveEvent(pEvent);
}

void WTrainingBpm::setRevealed(bool revealed) {
    if (!m_pTrainingMode->toBool()) {
        revealed = false;
    }
    if (m_revealed == revealed) {
        return;
    }
    m_revealed = revealed;
    refreshText();
}

void WTrainingBpm::refreshText() {
    if (m_pTrainingMode->toBool() && !m_revealed) {
        setText(m_skinText.contains("%1") ? m_skinText.arg(QStringLiteral("?.?"))
                                           : m_skinText + QStringLiteral("?.?"));
        return;
    }
    WNumber::setValue(m_value);
}
