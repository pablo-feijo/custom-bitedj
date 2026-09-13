#pragma once

#include "widget/wnumber.h"

class ControlProxy;

// A BPM number that masks only its numeric value while BiteDJ training mode is
// enabled. Pressing and holding the widget reveals the live value; every
// release/cancel/hide path masks it again.
class WTrainingBpm final : public WNumber {
    Q_OBJECT
  public:
    explicit WTrainingBpm(QWidget* pParent = nullptr);

    void setValue(double value) override;

  protected:
    bool event(QEvent* pEvent) override;
    void mousePressEvent(QMouseEvent* pEvent) override;
    void mouseReleaseEvent(QMouseEvent* pEvent) override;
    void leaveEvent(QEvent* pEvent) override;

  private:
    void setRevealed(bool revealed);
    void refreshText();

    ControlProxy* m_pTrainingMode;
    double m_value = 0.0;
    bool m_revealed = false;
};
