#pragma once

#include <QScrollArea>

#include "widget/wbasewidget.h"

class QDomNode;
class SkinContext;

/// Creates a QScrollArea. The QT default is to show scrollbars if needed.
class WScrollable : public QScrollArea, public WBaseWidget {
    Q_OBJECT
  public:
    WScrollable(QWidget* pParent);

    /// Supports scrollbar policies, optional TouchScroll and ResetOnEffectChange group.
    void setup(const QDomNode& node, const SkinContext& context);
};
