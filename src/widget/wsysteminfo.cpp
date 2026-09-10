#include "widget/wsysteminfo.h"

#include <QDateTime>
#include <QApplication>
#include <QTimeZone>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include "widget/wsystemdialogs.h"
#include <QVBoxLayout>
#include <QtConcurrentRun>

#include "control/controlobject.h"
#include "moc_wsysteminfo.cpp"
#include "preferences/audiodevicesettings.h"

namespace {
double control(const char* group, const char* item) {
    return ControlObject::get(ConfigKey(QString::fromLatin1(group), QString::fromLatin1(item)));
}
QString percent(std::optional<double> value) {
    return value ? QString::number(*value, 'f', 0) + QStringLiteral(" %")
                 : QStringLiteral("N/A");
}
} // namespace

WSystemInfo::WSystemInfo(QWidget* parent) : QWidget(parent), WBaseWidget(this) {
    // This dashboard contains native Qt controls. WWidget's touch-to-mouse
    // translation targets the container itself and bypasses the clock button.
    // Let Qt synthesize mouse events for the actual child under the finger.
    auto* layout = new QGridLayout(this);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(12);
    auto card = [this, layout](const QString& title, const QString& detail, int row, int col, bool editable = false) {
        QWidget* frame = editable ? static_cast<QWidget*>(new QPushButton(this)) : new QWidget(this);
        frame->setObjectName(QStringLiteral("InfoCard"));
        frame->setAttribute(Qt::WA_StyledBackground);
        frame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        auto* contents = new QVBoxLayout(frame);
        contents->setContentsMargins(16, 10, 16, 10);
        contents->setSpacing(2);
        auto* heading = new QLabel(title, frame);
        heading->setObjectName(QStringLiteral("InfoHeading"));
        auto* value = new QLabel(QStringLiteral("N/A"), frame);
        value->setObjectName(QStringLiteral("InfoValue"));
        auto* hint = new QLabel(detail, frame);
        hint->setObjectName(QStringLiteral("InfoHint"));
        if (editable) {
            frame->setObjectName(QStringLiteral("InfoClockCard"));
            frame->setCursor(Qt::PointingHandCursor);
            frame->setAccessibleName(tr("Edit local date and time"));
            heading->setAttribute(Qt::WA_TransparentForMouseEvents);
            value->setAttribute(Qt::WA_TransparentForMouseEvents);
            hint->setAttribute(Qt::WA_TransparentForMouseEvents);
            connect(static_cast<QPushButton*>(frame), &QPushButton::clicked, this,
                    [this] { mixxx::systemdialogs::clock(this); });
            m_date = hint;
        }
        contents->addWidget(heading);
        contents->addWidget(value, 1);
        contents->addWidget(hint);
        layout->addWidget(frame, row, col);
        return value;
    };
    m_load = card(tr("AUDIO LOAD"), tr("Audio callback utilization"), 0, 0);
    m_clock = card(tr("LOCAL TIME"), tr("Tap to edit date & time"), 0, 1, true);
    m_cpu = card(tr("CPU"), tr("Whole-system utilization"), 1, 0);
    m_temperature = card(tr("TEMPERATURE"), tr("CPU / SoC sensor"), 1, 1);
    m_output = new QLabel(this);
    m_output->setObjectName(QStringLiteral("InfoOutput"));
    m_output->setWordWrap(true);
    layout->addWidget(m_output, 2, 0, 1, 2);
    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 1);
    layout->setRowStretch(0, 1);
    layout->setRowStretch(1, 1);
    connect(&m_watcher, &QFutureWatcher<mixxx::systemtelemetry::Snapshot>::finished,
            this, [this]() {
                const auto result = m_watcher.result();
                m_cpu->setText(percent(m_previousCpu && result.cpu
                                ? mixxx::systemtelemetry::cpuUsage(*m_previousCpu, *result.cpu)
                                : std::nullopt));
                m_previousCpu = result.cpu;
                m_temperature->setText(result.temperature
                                ? QString::number(*result.temperature, 'f', 0) + tr(" °C")
                                : tr("N/A"));
                m_age.restart();
            });
    connect(&m_timer, &QTimer::timeout, this, &WSystemInfo::refresh);
    m_timer.start(1000);
    m_age.start();
    refresh();
}

void WSystemInfo::setup(const QDomNode&, const SkinContext&) {
}

void WSystemInfo::refresh() {
    const auto zoneId = qApp->property("bitedjTimeZone").toByteArray();
    const auto now = zoneId.isEmpty() ? QDateTime::currentDateTime()
            : QDateTime::currentDateTimeUtc().toTimeZone(QTimeZone(zoneId));
    m_clock->setText(now.toString(QStringLiteral("HH:mm")));
    m_date->setText(now.toString(QStringLiteral("ddd, d MMM yyyy")) + tr(" · Tap to edit"));
    const bool outputEnabled = control("[Master]", "main_output_connected") > 0;
    const bool audioEnabled = outputEnabled || control("[Master]", "headEnabled") > 0 ||
            control("[Master]", "booth_enabled") > 0;
    m_load->setText(audioEnabled
                    ? percent(control("[App]", "audio_latency_usage") * 100.0)
                    : tr("N/A"));
    if (auto* settings = AudioDeviceSettings::tryInstance()) {
        const QString assignment = settings->busLabels().value(AudioDeviceSettings::BusMaster);
        const bool pending = control("[AudioDevices]", "dirty") > 0;
        m_output->setText(pending ? tr("Pending output setting: %1 — apply in AUDIO").arg(assignment)
                                 : tr("%1 · %2").arg(assignment,
                                           outputEnabled ? tr("OUTPUT ENABLED") : tr("OUTPUT DISABLED")));
    } else {
        m_output->setText(tr("Main output: N/A"));
    }
    if (m_age.elapsed() > 5000) {
        m_cpu->setText(tr("N/A"));
        m_temperature->setText(tr("N/A"));
        m_previousCpu.reset();
    }
    if (!m_watcher.isRunning()) {
        // No widget captured by the worker. A skin reload may destroy this
        // widget while /proc or sysfs is being read.
        m_watcher.setFuture(QtConcurrent::run(mixxx::systemtelemetry::readSnapshot));
    }
}
