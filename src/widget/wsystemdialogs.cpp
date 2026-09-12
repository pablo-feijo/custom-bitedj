// Copyright (C) 2026 Custom Bite DJ contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#include "widget/wsystemdialogs.h"
#include <QApplication>
#include <QCheckBox>
#include <QCalendarWidget>
#include <QComboBox>
#include <QListView>
#include <QScroller>
#include <QTimeZone>
#include <QSignalBlocker>
#include <QTextCharFormat>
#include <QToolButton>
#include "skin/highcontrast.h"
#include <QCloseEvent>
#include <QDateTime>
#include <QDialog>
#include <QFutureWatcher>
#include <QHBoxLayout>
#include <QLabel>
#include <QProcess>
#include <QPushButton>
#include <QSpinBox>
#include <QTimer>
#include <QPointer>
#include <QShowEvent>
#include <memory>
#include <QVBoxLayout>
#include <QtConcurrentRun>
#include <functional>
#include "util/bootsettings.h"
#include "util/timezonecities.h"
#include <QLocale>
#include <QMainWindow>
#ifdef Q_OS_LINUX
#include <unistd.h>
#endif

namespace mixxx::systemdialogs {
bool requiresSudo(const QString& program, const QStringList& args, bool runningAsRoot) {
    if (runningAsRoot || args.isEmpty()) {
        return false;
    }
    if (program == QStringLiteral("timedatectl")) {
        return args.first() != QStringLiteral("show");
    }
    if (program == QStringLiteral("systemctl")) {
        return args.first() != QStringLiteral("is-enabled") &&
                args.first() != QStringLiteral("is-active") &&
                args.first() != QStringLiteral("show") &&
                args.first() != QStringLiteral("status");
    }
    return false;
}
namespace {
class OperationDialog : public QDialog {
  public:
    explicit OperationDialog(QWidget* owner)
            : QDialog(nullptr), m_ownerWindow(owner ? owner->window() : nullptr) {}
    ~OperationDialog() override {
        // QProcess destruction can emit finished while sibling controls are
        // already being destroyed. Never run UI callbacks during teardown.
        for (auto* process : findChildren<QProcess*>()) process->disconnect();
        restoreOwner();
    }
    void done(int result) override {
        QDialog::done(result);
        restoreOwner();
    }
    void reject() override { if (!property("busy").toBool()) QDialog::reject(); }
  protected:
    void showEvent(QShowEvent* event) override {
        // Sway hiding a window behind a fullscreen modal does not reliably unmap
        // Qt's embedded GL/native skin surfaces. Explicitly hide their QWidget
        // owner, then restore it when the modal completes.
        if (m_ownerWindow && m_ownerWindow->isVisible() && !m_restoreOwner) {
            m_ownerState = m_ownerWindow->windowState();
            m_restoreOwner = true;
            m_ownerWindow->hide();
        }
        QDialog::showEvent(event);
    }
    void closeEvent(QCloseEvent* event) override {
        if (property("busy").toBool()) event->ignore();
        else QDialog::closeEvent(event);
    }
  private:
    void restoreOwner() {
        if (!m_restoreOwner) return;
        m_restoreOwner = false;
        if (m_ownerWindow && !QCoreApplication::closingDown()) {
            m_ownerWindow->setWindowState(m_ownerState);
            m_ownerWindow->show();
        }
    }
    QPointer<QWidget> m_ownerWindow;
    Qt::WindowStates m_ownerState;
    bool m_restoreOwner = false;
};
QString tr(const char* text) { return QCoreApplication::translate("SystemDialogs", text); }
QPushButton* button(const QString& text, QLayout* layout, const std::function<void()>& action) {
    auto* b = new QPushButton(text);
    b->setObjectName(QStringLiteral("SystemDialogButton"));
    b->setMinimumHeight(48);
    layout->addWidget(b);
    QObject::connect(b, &QPushButton::clicked, b, action);
    return b;
}
QDialog* dialog(QWidget* parent, const QString& title, QVBoxLayout** layout) {
    // System actions originate at the main window; their theme belongs to the skin.
    if (auto* main = qobject_cast<QMainWindow*>(parent)) parent = main->centralWidget();
    // Independent native windows match Preferences. Parenting to the skin on
    // Wayland can put its embedded GL surfaces above the modal's contents.
    auto* d = new OperationDialog(parent);
    if (parent) {
        QString styles;
        for (auto* widget = parent; widget; widget = widget->parentWidget())
            styles.prepend(widget->styleSheet() + QLatin1Char('\n'));
        d->setStyleSheet(styles);
        d->setFont(parent->font());
        d->setPalette(parent->palette());
        QObject::connect(parent, &QObject::destroyed, d, &QObject::deleteLater);
    }
    d->setAttribute(Qt::WA_DeleteOnClose);
    d->setObjectName(QStringLiteral("SystemDialog"));
    d->setWindowTitle(title);
    d->setWindowModality(Qt::ApplicationModal);
    *layout = new QVBoxLayout(d);
    (*layout)->setContentsMargins(28, 20, 28, 20);
    (*layout)->setSpacing(12);
    auto* heading = new QLabel(title, d);
    heading->setObjectName(QStringLiteral("SystemDialogTitle"));
    (*layout)->addWidget(heading);
    return d;
}
QLabel* label(const QString& text, QLayout* layout) {
    auto* l = new QLabel(text);
    l->setWordWrap(true);
    layout->addWidget(l);
    return l;
}
QSpinBox* spin(const QString& title, int value, int low, int high, QVBoxLayout* layout) {
    auto* fieldWidget = new QWidget;
    auto* row = new QHBoxLayout(fieldWidget);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(12);
    label(title, row)->setMinimumWidth(150);
    auto* s = new QSpinBox;
    s->setAccessibleName(title);
    s->setRange(low, high);
    s->setValue(value);
    s->setMinimumHeight(48);
    s->setButtonSymbols(QAbstractSpinBox::NoButtons);
    s->setReadOnly(true);
    button(QStringLiteral("−"), row, [s] { if (!s->isEnabled()) return;
        if (s->property("clockFloor").toInt() > 0 && s->value() <= s->property("clockFloor").toInt()) s->setValue(0);
        else s->stepDown(); })->setFixedWidth(64);
    row->addWidget(s, 1);
    button(QStringLiteral("+"), row, [s] { if (!s->isEnabled()) return;
        if (s->value() == 0 && s->property("clockFloor").toInt() > 0) s->setValue(s->property("clockFloor").toInt());
        else s->stepUp(); })->setFixedWidth(64);
    layout->addWidget(fieldWidget);
    return s;
}
// No shell, no GUI-thread waits, and report both failed starts and nonzero exits.
void command(QObject* owner, const QString& program, const QStringList& args,
        const std::function<void(bool, QString)>& done) {
    auto* p = new QProcess(owner);
    auto delivered = std::make_shared<bool>(false);
    auto finish = [p, delivered, done](bool ok, const QString& message) {
        if (*delivered) return;
        *delivered = true;
        done(ok, message);
        p->deleteLater();
    };
    QObject::connect(p, &QProcess::errorOccurred, p, [finish](QProcess::ProcessError e) {
        if (e == QProcess::FailedToStart) finish(false, tr("System service could not be started."));
    });
    QObject::connect(p, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), p,
            [p, finish](int code, QProcess::ExitStatus status) {
                const bool ok = code == 0 && status == QProcess::NormalExit;
                QByteArray message = ok ? p->readAllStandardOutput() : p->readAllStandardError();
                if (message.trimmed().isEmpty()) {
                    message = ok ? p->readAllStandardError() : p->readAllStandardOutput();
                }
                finish(ok, QString::fromUtf8(message).trimmed());
            });
    QTimer::singleShot(15000, p, [p, finish] {
        p->kill();
        finish(false, tr("System service timed out. Check the current setting before retrying."));
    });
#ifdef Q_OS_LINUX
    // The Pi image runs as pi and grants passwordless sudo. Mutations and power
    // requests otherwise depend on a Polkit agent, which the touch UI has none.
    if (requiresSudo(program, args, geteuid() == 0)) {
        p->start(QStringLiteral("sudo"), QStringList{QStringLiteral("-n"), program} + args);
        return;
    }
#endif
    p->start(program, args);
}
void confirmPower(QWidget* parent, const QString& action, const QString& program) {
    QVBoxLayout* layout;
    auto* d = dialog(parent, action + QStringLiteral("?"), &layout);
    label(tr("Playback and recording will stop. Save your work before continuing."), layout);
    layout->addStretch();
    auto* status = label(QString(), layout);
    auto* row = new QHBoxLayout;
    layout->addLayout(row);
    auto* cancel = button(tr("Cancel"), row, [d] { d->close(); });
    auto* apply = button(action, row, [] {});
    QObject::connect(apply, &QPushButton::clicked, d, [=] {
        d->setProperty("busy", true);
        apply->setEnabled(false);
        cancel->setEnabled(false);
        if (program.isEmpty()) {
            // main() relaunches only after CoreServices has flushed and torn down.
            qApp->setProperty("bitedjRestart", true);
            QCoreApplication::exit(0);
            return;
        }
        status->setText(tr("Sending system request…"));
        command(d, QStringLiteral("systemctl"), {program}, [=](bool ok, const QString& error) {
            d->setProperty("busy", false);
            status->setText(ok ? tr("System request accepted.") : tr("Request failed: %1").arg(error));
            apply->setEnabled(!ok);
            cancel->setEnabled(true);
        });
    });
    d->showFullScreen();
}
}
void power(QWidget* parent) {
    QVBoxLayout* layout;
    auto* d = dialog(parent, tr("Power & restart"), &layout);
    label(tr("Restart BiteDJ to reload the application. Restart the system to apply boot settings."), layout);
    layout->addStretch();
    button(tr("Restart BiteDJ"), layout, [d] { confirmPower(d, tr("Restart BiteDJ"), {}); });
    button(tr("Restart system"), layout, [d] { confirmPower(d, tr("Restart system"), QStringLiteral("reboot")); });
    button(tr("Power off"), layout, [d] { confirmPower(d, tr("Power off"), QStringLiteral("poweroff")); });
    button(tr("Back"), layout, [d] { d->close(); });
    d->showFullScreen();
}
void ssh(QWidget* parent) {
    QVBoxLayout* layout;
    auto* d = dialog(parent, tr("SSH remote access"), &layout);
    auto* status = label(tr("Checking the SSH service…"), layout);
    label(tr("Enable SSH for key-based remote maintenance. Disabling it immediately ends remote access."), layout);
    layout->addStretch();
    auto* enable = button(tr("Enable SSH"), layout, [] {});
    auto* disable = button(tr("Disable SSH"), layout, [] {});
    auto* back = button(tr("Back"), layout, [d] { d->close(); });
    enable->setEnabled(false);
    disable->setEnabled(false);

    auto setState = [=](bool enabled) {
        d->setProperty("busy", false);
        status->setText(enabled
                        ? tr("SSH is enabled. Remote access requires an authorized key.")
                        : tr("SSH is disabled. This device is not accepting remote logins."));
        enable->setEnabled(!enabled);
        disable->setEnabled(enabled);
        back->setEnabled(true);
    };
    auto apply = [=](bool enabled) {
        d->setProperty("busy", true);
        enable->setEnabled(false);
        disable->setEnabled(false);
        back->setEnabled(false);
        status->setText(enabled ? tr("Enabling SSH…") : tr("Disabling SSH…"));
        command(d,
                QStringLiteral("systemctl"),
                {enabled ? QStringLiteral("enable") : QStringLiteral("disable"),
                        QStringLiteral("--now"),
                        QStringLiteral("ssh.service")},
                [=](bool ok, const QString& error) {
                    if (ok) {
                        setState(enabled);
                        return;
                    }
                    d->setProperty("busy", false);
                    status->setText(tr("Could not change SSH: %1").arg(
                            error.isEmpty() ? tr("unknown system service error") : error));
                    enable->setEnabled(true);
                    disable->setEnabled(true);
                    back->setEnabled(true);
                });
    };
    QObject::connect(enable, &QPushButton::clicked, d, [=] { apply(true); });
    QObject::connect(disable, &QPushButton::clicked, d, [=] { apply(false); });
    command(d,
            QStringLiteral("systemctl"),
            {QStringLiteral("is-enabled"), QStringLiteral("ssh.service")},
            [=](bool ok, const QString& output) {
                if (ok) {
                    setState(true);
                } else if (output.trimmed() == QStringLiteral("disabled")) {
                    setState(false);
                } else {
                    status->setText(tr("SSH service unavailable: %1").arg(
                            output.isEmpty() ? tr("not installed") : output));
                    d->setProperty("busy", false);
                    back->setEnabled(true);
                }
            });
    d->showFullScreen();
}
void clock(QWidget* parent) {
    QVBoxLayout* layout;
    auto* d = dialog(parent, tr("Local date & time"), &layout);
    auto* status = label(tr("Choose your timezone. The hour and date follow automatically."), layout);
    auto* preview = label(QString(), layout);
    preview->setObjectName(QStringLiteral("ClockPreview"));
    auto combo = [=](const QString& name) {
        auto* row = new QHBoxLayout;
        label(name, row)->setMinimumWidth(150);
        auto* box = new QComboBox(d);
        box->setAccessibleName(name);
        box->setMinimumHeight(48);
        box->setView(new QListView(box));
        QScroller::grabGesture(box->view()->viewport(), QScroller::TouchGesture);
        box->setMaxVisibleItems(7);
        row->addWidget(box, 1); layout->addLayout(row);
        return box;
    };
    auto* region = combo(tr("Country"));
    auto* city = combo(tr("Main city"));
    auto zones = std::make_shared<QMap<QString, QList<QByteArray>>>();
    (*zones)[QStringLiteral("UTC")].append(QByteArray("Etc/UTC"));
    for (const auto& entry : mixxx::timezones::kMainCities) {
        const QByteArray id(entry.id);
        if (!QTimeZone(id).isValid()) continue;
        const auto territory = QLocale::codeToTerritory(QString::fromLatin1(entry.territory));
        (*zones)[QLocale::territoryToString(territory)].append(id);
    }
    region->addItems(zones->keys());
    auto* automatic = new QCheckBox(tr("Set time automatically from the internet"), d);
    automatic->setMinimumHeight(44); layout->addWidget(automatic);
    auto* manual = new QWidget(d);
    auto* manualLayout = new QVBoxLayout(manual);
    manualLayout->setContentsMargins(0, 0, 0, 0);
    auto* date = button(QString(), manualLayout, [] {});
    date->setObjectName(QStringLiteral("ClockDateButton"));
    auto* timeRow = new QHBoxLayout;
    auto* hours = new QVBoxLayout;
    auto* minutes = new QVBoxLayout;
    timeRow->addLayout(hours); timeRow->addLayout(minutes); manualLayout->addLayout(timeRow);
    auto* hour = spin(tr("Hour"), 0, 0, 23, hours);
    auto* minute = spin(tr("Minute"), 0, 0, 59, minutes);
    hour->setWrapping(true); minute->setWrapping(true);
    layout->addWidget(manual);
    auto selectedDate = std::make_shared<QDate>();
    auto dirty = std::make_shared<bool>(false);
    auto currentZone = std::make_shared<QString>();
    auto refresh = [=] {
        const QTimeZone zone(city->currentData().toByteArray());
        if (!zone.isValid()) return;
        const auto local = QDateTime::currentDateTimeUtc().toTimeZone(zone);
        preview->setText(local.toString(QStringLiteral("ddd, d MMM yyyy · HH:mm · t")));
        if (!*dirty) {
            *selectedDate = local.date();
            const QSignalBlocker h(hour), m(minute);
            hour->setValue(local.time().hour()); minute->setValue(local.time().minute());
        }
        date->setText(selectedDate->toString(QStringLiteral("ddd, d MMM yyyy")) + tr(" · Change date"));
    };
    auto populate = [=] {
        const QSignalBlocker block(city);
        city->clear();
        for (const auto& id : zones->value(region->currentText())) {
            QString text = QString::fromUtf8(id);
            if (text.contains('/')) text = text.section('/', -1);
            city->addItem(text.replace('_', ' '), id);
        }
        refresh();
    };
    QObject::connect(region, qOverload<int>(&QComboBox::currentIndexChanged), d, populate);
    QObject::connect(city, qOverload<int>(&QComboBox::currentIndexChanged), d, refresh);
    auto selectZone = [=](const QByteArray& id) {
        QString country;
        for (auto it = zones->cbegin(); it != zones->cend(); ++it)
            if (it.value().contains(id)) { country = it.key(); break; }
        if (country.isEmpty()) {
            // Retain a configured timezone outside the shortlist without exposing
            // every historical alias or changing it merely by opening this editor.
            const QTimeZone zone(id);
            country = zone.territory() == QLocale::AnyTerritory
                    ? tr("Current timezone") : QLocale::territoryToString(zone.territory());
            (*zones)[country].append(id);
            if (region->findText(country) < 0) region->addItem(country);
        }
        region->setCurrentText(country);
        populate(); city->setCurrentIndex(city->findData(id)); refresh();
    };
    selectZone(QTimeZone::systemTimeZoneId());
    QObject::connect(hour, qOverload<int>(&QSpinBox::valueChanged), d, [=] { *dirty = true; });
    QObject::connect(minute, qOverload<int>(&QSpinBox::valueChanged), d, [=] { *dirty = true; });
    QObject::connect(automatic, &QCheckBox::toggled, d, [=](bool enabled) {
        manual->setVisible(!enabled);
        if (enabled) { *dirty = false; refresh(); }
    });
    QObject::connect(date, &QPushButton::clicked, d, [=] {
        QVBoxLayout* calendarLayout;
        auto* picker = dialog(d, tr("Choose date"), &calendarLayout);
        auto* calendar = new QCalendarWidget(picker);
        calendar->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
        QTextCharFormat header;
        header.setForeground(HighContrast::mapColor(QColor(QStringLiteral("#e5e6ea"))));
        header.setBackground(HighContrast::mapColor(QColor(QStringLiteral("#252530"))));
        calendar->setHeaderTextFormat(header);
        for (const auto& name : {QStringLiteral("qt_calendar_prevmonth"), QStringLiteral("qt_calendar_nextmonth")}) {
            if (auto* arrow = calendar->findChild<QToolButton*>(name)) {
                arrow->setToolButtonStyle(Qt::ToolButtonTextOnly);
                arrow->setText(name.endsWith(QStringLiteral("prevmonth")) ? QStringLiteral("‹") : QStringLiteral("›"));
            }
        }
        calendar->setSelectedDate(*selectedDate);
        calendarLayout->addWidget(calendar, 1);
        auto* actions = new QHBoxLayout; calendarLayout->addLayout(actions);
        button(tr("Cancel"), actions, [picker] { picker->close(); });
        button(tr("Today"), actions, [=] {
            calendar->setSelectedDate(QDateTime::currentDateTimeUtc().toTimeZone(QTimeZone(city->currentData().toByteArray())).date());
        });
        button(tr("Use date"), actions, [=] {
            *selectedDate = calendar->selectedDate(); *dirty = true; refresh(); picker->close();
        });
        picker->showFullScreen();
    });
    auto* timer = new QTimer(d);
    QObject::connect(timer, &QTimer::timeout, d, refresh); timer->start(1000);
    layout->addStretch();
    auto* row = new QHBoxLayout; layout->addLayout(row);
    auto* cancel = button(tr("Cancel"), row, [d] { d->close(); });
    auto* apply = button(tr("Apply"), row, [] {});
    apply->setEnabled(false);
    command(d, QStringLiteral("timedatectl"), {QStringLiteral("show"), QStringLiteral("--property=NTP"), QStringLiteral("--property=Timezone")},
            [=](bool ok, const QString& output) {
                QString ntp, timezone;
                for (const auto& line : output.split('\n')) {
                    if (line.startsWith(QStringLiteral("NTP="))) ntp = line.mid(4);
                    if (line.startsWith(QStringLiteral("Timezone="))) timezone = line.mid(9);
                }
                const bool known = ok && (ntp == QStringLiteral("yes") || ntp == QStringLiteral("no")) && QTimeZone(timezone.toUtf8()).isValid();
                automatic->setChecked(ntp == QStringLiteral("yes"));
                manual->setVisible(!automatic->isChecked());
                if (known) { *currentZone = timezone; selectZone(timezone.toUtf8()); }
                else status->setText(tr("Date/time service unavailable. Changes cannot be applied here."));
                apply->setEnabled(known);
            });
    QObject::connect(apply, &QPushButton::clicked, d, [=] {
        const QByteArray zoneId = city->currentData().toByteArray();
        const QTimeZone zone(zoneId);
        const QTime requestedTime(hour->value(), minute->value());
        const QDateTime value(*selectedDate, requestedTime, zone);
        const bool manualChange = !automatic->isChecked() && *dirty;
        if (!zone.isValid() || (manualChange && (!value.isValid() || value.date() != *selectedDate || value.time() != requestedTime))) {
            status->setText(tr("That local time does not exist in this timezone. Choose another time.")); return;
        }
        d->setProperty("busy", true); timer->stop();
        apply->setEnabled(false); cancel->setEnabled(false); region->setEnabled(false); city->setEnabled(false);
        automatic->setEnabled(false); manual->setEnabled(false);
        auto done = [=](bool ok, const QString& error) {
            d->setProperty("busy", false);
            status->setText(ok ? tr("Date/time settings applied.") : tr("Could not apply: %1").arg(error));
            if (ok) *dirty = false;
            cancel->setText(tr("Done")); cancel->setEnabled(true); apply->setEnabled(true);
            region->setEnabled(true); city->setEnabled(true); automatic->setEnabled(true); manual->setEnabled(true);
            refresh(); timer->start();
        };
        auto applyTime = [=] {
            command(d, QStringLiteral("timedatectl"), {QStringLiteral("set-ntp"), automatic->isChecked() ? QStringLiteral("true") : QStringLiteral("false")},
                    [=](bool ok, const QString& error) {
                        if (!ok || !manualChange) { done(ok, error); return; }
                        command(d, QStringLiteral("timedatectl"), {QStringLiteral("set-time"), value.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))},
                                [=](bool success, const QString& message) {
                                    done(success, success ? message : tr("Automatic sync is off; time was not changed. %1").arg(message));
                                });
                    });
        };
        if (QString::fromUtf8(zoneId) == *currentZone) { applyTime(); return; }
        command(d, QStringLiteral("timedatectl"), {QStringLiteral("set-timezone"), QString::fromUtf8(zoneId)},
                [=](bool ok, const QString& error) {
                    if (!ok) { done(false, error); return; }
                    *currentZone = QString::fromUtf8(zoneId);
                    qApp->setProperty("bitedjTimeZone", zoneId);
                    status->setText(tr("Timezone updated. Applying time sync…"));
                    applyTime();
                });
    });
    d->showFullScreen();
}
void overclock(QWidget* parent, std::function<bootsettings::Settings()> reader) {
    QVBoxLayout* layout;
    auto* d = dialog(parent, tr("Overclock settings"), &layout);
    auto* status = label(tr("Reading boot settings…"), layout);
    auto* cpu = spin(tr("CPU · MHz"), 0, 0, 3000, layout); cpu->setSingleStep(100); cpu->setProperty("clockFloor", 1000);
    auto* gpu = spin(tr("GPU · MHz"), 0, 0, 1000, layout); gpu->setSingleStep(50); gpu->setProperty("clockFloor", 400);
    auto* voltage = spin(tr("Voltage offset"), 0, 0, 6, layout);
    cpu->setSpecialValueText(tr("Firmware default")); gpu->setSpecialValueText(tr("Firmware default"));
    voltage->setSpecialValueText(tr("Automatic"));
    label(tr("Higher clocks can cause heat, instability or boot failure. Use adequate cooling. Voltage steps are 25 mV. Zero uses firmware defaults."), layout);
    layout->addStretch();
    auto* row = new QHBoxLayout; layout->addLayout(row);
    auto* back = button(tr("Back"), row, [d] { d->close(); });
    auto* defaults = button(tr("Firmware defaults"), row, [] {});
    auto* save = button(tr("Save for next restart"), row, [] {});
    auto* restart = button(tr("Restart system…"), layout, [d] { confirmPower(d, tr("Restart system"), QStringLiteral("reboot")); });
    save->setEnabled(false); defaults->setEnabled(false); restart->setEnabled(false);
    auto* watcher = new QFutureWatcher<bootsettings::Settings>(d);
    QObject::connect(watcher, &QFutureWatcher<bootsettings::Settings>::finished, d, [=] {
        auto settings = std::make_shared<bootsettings::Settings>(watcher->result());
        const bool ok = settings->error.isEmpty();
        status->setText(ok ? tr("%1 · Saved boot values (may differ from the running clock)").arg(settings->model) : settings->error);
        cpu->setMaximum(settings->model.startsWith(QStringLiteral("Raspberry Pi 4")) ? 2400 : 3000);
        cpu->setValue(settings->cpu); gpu->setValue(settings->gpu); voltage->setValue(settings->voltage);
        cpu->parentWidget()->setEnabled(ok); gpu->parentWidget()->setEnabled(ok); voltage->parentWidget()->setEnabled(ok);
        save->setEnabled(ok); defaults->setEnabled(ok); restart->setEnabled(ok);
        const auto saveValues = [=](int c, int g, int v, bool firmwareDefaults) {
            if ((c && c < 1000) || (g && g < 400)) { status->setText(tr("Use CPU 1000 MHz or higher and GPU 400 MHz or higher, or choose firmware defaults.")); return; }
            d->setProperty("busy", true);
            cpu->parentWidget()->setEnabled(false); gpu->parentWidget()->setEnabled(false); voltage->parentWidget()->setEnabled(false);
            save->setEnabled(false); defaults->setEnabled(false); restart->setEnabled(false); back->setEnabled(false);
            auto* saved = new QFutureWatcher<QString>(d);
            QObject::connect(saved, &QFutureWatcher<QString>::finished, d, [=] {
                d->setProperty("busy", false);
                const QString error = saved->result();
                if (error.isEmpty()) settings->original = bootsettings::render(settings->original, c, g, v);
                status->setText(error.isEmpty()
                                ? (firmwareDefaults
                                                  ? tr("Firmware defaults saved. Restart the system to apply. Recovery copy: %1.bitedj-backup").arg(settings->path)
                                                  : tr("Saved. Restart the system to apply. Recovery copy: %1.bitedj-backup").arg(settings->path))
                                : error);
                cpu->parentWidget()->setEnabled(true); gpu->parentWidget()->setEnabled(true); voltage->parentWidget()->setEnabled(true);
                save->setEnabled(true); defaults->setEnabled(true); restart->setEnabled(true); back->setEnabled(true);
                saved->deleteLater();
            });
            saved->setFuture(QtConcurrent::run([snapshot = *settings, c, g, v] { return bootsettings::save(snapshot, c, g, v); }));
        };
        QObject::connect(save, &QPushButton::clicked, d, [=] {
            saveValues(cpu->value(), gpu->value(), voltage->value(), false);
        });
        QObject::connect(defaults, &QPushButton::clicked, d, [=] {
            cpu->setValue(0); gpu->setValue(0); voltage->setValue(0);
            saveValues(0, 0, 0, true);
        });
    });
    watcher->setFuture(QtConcurrent::run(std::move(reader)));
    d->showFullScreen();
}
} // namespace mixxx::systemdialogs
