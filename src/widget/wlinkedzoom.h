#pragma once
#include "widget/wwidget.h"
class QDomNode;
class SkinContext;
class WLinkedZoom : public WWidget {
    Q_OBJECT
  public:
    explicit WLinkedZoom(QWidget* parent = nullptr);
    void setup(const QDomNode&, const SkinContext&) {}
};
