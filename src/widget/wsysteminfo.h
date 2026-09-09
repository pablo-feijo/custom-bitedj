#pragma once

#include <QElapsedTimer>
#include <QFutureWatcher>
#include <QTimer>

#include "util/systemtelemetry.h"
#include "widget/wwidget.h"

class QLabel;
class SkinContext;
class QDomNode;

class WSystemInfo : public WWidget {
    Q_OBJECT
  public:
    explicit WSystemInfo(QWidget* parent = nullptr);
    void setup(const QDomNode&, const SkinContext&);

  private:
    void refresh();
    QLabel* m_load;
    QLabel* m_cpu;
    QLabel* m_temperature;
    QLabel* m_clock;
    QLabel* m_output;
    QTimer m_timer;
    QElapsedTimer m_age;
    QFutureWatcher<mixxx::systemtelemetry::Snapshot> m_watcher;
    std::optional<mixxx::systemtelemetry::CpuTicks> m_previousCpu;
};
