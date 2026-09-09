#pragma once
#include <QTimer>
#include "widget/wwidget.h"
class SkinContext;
class QDomNode;
class WOverviewRuler : public WWidget {
    Q_OBJECT
  public:
    explicit WOverviewRuler(QWidget* parent = nullptr);
    void setup(const QDomNode& node, const SkinContext& context);
  protected:
    void paintEvent(QPaintEvent*) override;
  private:
    QString m_group;
    QTimer m_timer;
};
