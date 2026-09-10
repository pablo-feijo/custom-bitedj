// Copyright (C) 2026 Custom Bite DJ contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#include <gtest/gtest.h>
#include <functional>
#include <QFile>
#include <QMainWindow>
#include <QCryptographicHash>
#include <QTemporaryDir>
#include "util/bootsettings.h"
using namespace mixxx::bootsettings;
namespace {
const QString model = QStringLiteral("Raspberry Pi 4 Model B Rev 1.4");
const QByteArray bootConfig("# retain attribution\narm_freq=2000\ngpu_freq=750\nover_voltage=6\ndtoverlay=vc4-kms-v3d\n[cm4]\notg_mode=1\n");
}
TEST(BootSettingsTest, ReadsImageDefaultsAndPreservesUnrelatedConfiguration) {
    auto settings = parse(bootConfig, model);
    ASSERT_TRUE(settings.error.isEmpty());
    EXPECT_EQ(settings.cpu, 2000);
    EXPECT_EQ(settings.gpu, 750);
    EXPECT_EQ(settings.voltage, 6);
    auto changed = render(bootConfig, 1800, 600, 2);
    EXPECT_TRUE(changed.contains("# retain attribution"));
    EXPECT_TRUE(changed.contains("[cm4]\notg_mode=1"));
    EXPECT_TRUE(changed.contains("[all]\n# Custom Bite DJ clock settings\narm_freq=1800"));
    auto reread = parse(changed, model);
    EXPECT_TRUE(reread.error.isEmpty());
    EXPECT_EQ(reread.cpu, 1800);
    EXPECT_EQ(render(changed, 1800, 600, 2), changed);
    EXPECT_TRUE(render(changed + "dtparam=audio=on\n", 1900, 600, 2).contains("dtparam=audio=on"));
}
TEST(BootSettingsTest, DefaultsRemoveOnlyManagedOverrides) {
    auto changed = render(bootConfig, 0, 0, 0);
    EXPECT_FALSE(changed.contains("arm_freq="));
    EXPECT_FALSE(changed.contains("gpu_freq="));
    EXPECT_FALSE(changed.contains("over_voltage="));
    EXPECT_TRUE(changed.contains("dtoverlay=vc4-kms-v3d"));
}
TEST(BootSettingsTest, RejectsUnknownBoardsAdvancedOverridesAndUnsafeValues) {
    EXPECT_FALSE(parse(bootConfig, QStringLiteral("Desktop")).error.isEmpty());
    for (auto extra : {"include clocks.txt\n", "force_turbo=1\n", "over_voltage_delta=50000\n", "[HDMI:0]\narm_freq=2200\n"}) {
        EXPECT_FALSE(parse(bootConfig + extra, model).error.isEmpty());
    }
    EXPECT_FALSE(parse("arm_freq=9999\n", model).error.isEmpty());
    EXPECT_FALSE(parse("arm_freq=bad\n", model).error.isEmpty());
    EXPECT_FALSE(parse("core_freq=750\n", model).error.isEmpty());
}
TEST(BootSettingsTest, SavesBackupAndRefusesStaleWrites) {
    QTemporaryDir dir;
    auto settings = parse(bootConfig, model);
    settings.path = dir.filePath(QStringLiteral("config.txt"));
    QFile file(settings.path);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    file.write(bootConfig); file.close();
    ASSERT_TRUE(save(settings, 1800, 600, 2).isEmpty());
    QFile backup(settings.path + QStringLiteral(".bitedj-backup"));
    ASSERT_TRUE(backup.open(QIODevice::ReadOnly));
    EXPECT_EQ(backup.readAll(), bootConfig);
    EXPECT_FALSE(save(settings, 1900, 600, 2).isEmpty());
    ASSERT_TRUE(file.open(QIODevice::ReadOnly));
    EXPECT_EQ(file.readAll(), render(bootConfig, 1800, 600, 2));
}

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QCalendarWidget>
#include <QTimeZone>
#include <QDialog>
#include <QLabel>
#include <QElapsedTimer>
#include <QPushButton>
#include <QPointer>
#include <QProcess>
#include <QSpinBox>
#include <QThread>
#include <QTimer>
#include "test/mixxxtest.h"
#include "widget/wsystemdialogs.h"
#include "widget/wsysteminfo.h"
#include <QtTest/QTest>

class SystemDialogsTest : public MixxxTest {
  protected:
    QTemporaryDir commands;
    QByteArray oldPath;
    void SetUp() override {
        oldPath = qgetenv("PATH");
        qputenv("PATH", commands.path().toUtf8());
        qputenv("BITEDJ_COMMAND_LOG", commands.filePath("commands.log").toUtf8());
        QFile fake(commands.filePath("timedatectl"));
        ASSERT_TRUE(fake.open(QIODevice::WriteOnly));
        fake.write("#!/bin/sh\nprintf '%s\\n' \"$*\" >> \"$BITEDJ_COMMAND_LOG\"\nif [ \"$1\" = show ]; then printf 'NTP=yes\\nTimezone=Etc/UTC\\n'; fi\n");
        fake.close();
        fake.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
        QFile sudoCommand(commands.filePath("sudo"));
        ASSERT_TRUE(sudoCommand.open(QIODevice::WriteOnly));
        sudoCommand.write("#!/bin/sh\n[ \"$1\" = -n ] || exit 91\nshift\nexec \"$@\"\n");
        sudoCommand.close();
        sudoCommand.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
    }
    void TearDown() override {
        QList<QPointer<QWidget>> dialogs;
        for (auto* widget : QApplication::topLevelWidgets())
            if (widget->objectName() == QStringLiteral("SystemDialog")) dialogs.push_back(widget);
        for (const auto& widget : dialogs) if (widget) delete widget;
        qputenv("PATH", oldPath);
        qunsetenv("BITEDJ_COMMAND_LOG");
    }
    static bool until(const std::function<bool()>& ready) {
        QElapsedTimer timer; timer.start();
        while (!ready() && timer.elapsed() < 3000) {
            QCoreApplication::processEvents(); QThread::msleep(5);
        }
        return ready();
    }
    static QPushButton* findButton(QWidget* parent, const QString& text) {
        if (!parent) return nullptr;
        for (auto* b : parent->findChildren<QPushButton*>()) if (b->text() == text) return b;
        return nullptr;
    }
};
TEST_F(SystemDialogsTest, TouchClockCardOpensEditorAndCancelRestoresDashboard) {
    WSystemInfo info;
    info.resize(1024, 440);
    info.show();
    QTest::qWait(20);
    auto* card = info.findChild<QPushButton*>(QStringLiteral("InfoClockCard"));
    ASSERT_NE(card, nullptr);
    auto* value = card->findChild<QLabel*>(QStringLiteral("InfoValue"));
    ASSERT_NE(value, nullptr);
    // Hit the displayed time, including its child-label event routing. A direct
    // QPushButton::click() cannot catch a container swallowing touchscreen taps.
    auto* device = QTest::createTouchDevice();
    QTest::touchEvent(&info, device).press(0, value->rect().center(), value);
    QTest::touchEvent(&info, device).release(0, value->rect().center(), value);
    QPointer<QWidget> editor;
    ASSERT_TRUE(until([&] {
        for (auto* widget : QApplication::topLevelWidgets()) {
            if (widget->objectName() == QStringLiteral("SystemDialog") && widget->isVisible()) {
                editor = widget;
                return true;
            }
        }
        return false;
    }));
    EXPECT_EQ(editor->windowTitle(), QStringLiteral("Local date & time"));
    EXPECT_FALSE(info.isVisible());
    auto* cancel = findButton(editor, QStringLiteral("Cancel"));
    ASSERT_NE(cancel, nullptr);
    QTest::touchEvent(editor, device).press(0, cancel->rect().center(), cancel);
    QTest::touchEvent(editor, device).release(0, cancel->rect().center(), cancel);
    EXPECT_TRUE(until([&] { return info.isVisible() && (!editor || !editor->isVisible()); }));
}

TEST_F(SystemDialogsTest, ManualClockDisablesSyncBeforeChangingTime) {
    mixxx::systemdialogs::clock(nullptr);
    QCoreApplication::processEvents();
    auto* d = QApplication::activeWindow();
    ASSERT_NE(d, nullptr);
    auto* apply = findButton(d, QStringLiteral("Apply"));
    ASSERT_NE(apply, nullptr);
    ASSERT_TRUE(until([=] { return apply->isEnabled(); }));
    auto* automatic = d->findChild<QCheckBox*>();
    ASSERT_NE(automatic, nullptr);
    EXPECT_TRUE(automatic->isChecked());
    for (auto* field : d->findChildren<QSpinBox*>()) EXPECT_FALSE(field->isVisible());
    automatic->setChecked(false);
    d->findChildren<QSpinBox*>().last()->stepUp();
    apply->click();
    ASSERT_TRUE(until([=] { return apply->isEnabled(); }));
    QFile log(commands.filePath("commands.log"));
    ASSERT_TRUE(log.open(QIODevice::ReadOnly));
    const auto calls = log.readAll();
    EXPECT_TRUE(calls.contains("set-ntp false\nset-time "));
}
TEST_F(SystemDialogsTest, AutomaticClockNeverSendsManualDate) {
    mixxx::systemdialogs::clock(nullptr);
    QCoreApplication::processEvents();
    auto* d = QApplication::activeWindow();
    ASSERT_NE(d, nullptr);
    auto* apply = findButton(d, QStringLiteral("Apply"));
    ASSERT_NE(apply, nullptr);
    ASSERT_TRUE(until([=] { return apply->isEnabled(); }));
    apply->click();
    ASSERT_TRUE(until([=] { return apply->isEnabled(); }));
    QFile log(commands.filePath("commands.log"));
    ASSERT_TRUE(log.open(QIODevice::ReadOnly));
    const auto calls = log.readAll();
    EXPECT_TRUE(calls.contains("set-ntp true"));
    EXPECT_FALSE(calls.contains("set-time"));
}
TEST_F(SystemDialogsTest, PowerMenuRequiresConfirmationAndCancelDoesNothing) {
    mixxx::systemdialogs::power(nullptr);
    QCoreApplication::processEvents();
    auto* d = QApplication::activeWindow();
    ASSERT_NE(d, nullptr);
    auto* restart = findButton(d, QStringLiteral("Restart system"));
    ASSERT_NE(restart, nullptr);
    restart->click();
    QCoreApplication::processEvents();
    auto* confirmation = QApplication::activeWindow();
    ASSERT_NE(confirmation, d);
    auto* cancel = findButton(confirmation, QStringLiteral("Cancel"));
    ASSERT_NE(cancel, nullptr);
    cancel->click();
    EXPECT_FALSE(QFile::exists(commands.filePath("commands.log")));
    EXPECT_FALSE(qApp->property("bitedjRestart").toBool());
}
TEST_F(SystemDialogsTest, FailedManualTimeReportsThatSyncIsOff) {
    QFile fake(commands.filePath("timedatectl"));
    ASSERT_TRUE(fake.open(QIODevice::WriteOnly | QIODevice::Truncate));
    fake.write("#!/bin/sh\nif [ \"$1\" = show ]; then printf 'NTP=no\\nTimezone=Etc/UTC\\n'; fi\nif [ \"$1\" = set-time ]; then printf 'permission denied\\n' >&2; exit 1; fi\n");
    fake.close();
    mixxx::systemdialogs::clock(nullptr);
    QCoreApplication::processEvents();
    auto* d = QApplication::activeWindow();
    ASSERT_NE(d, nullptr);
    auto* apply = findButton(d, QStringLiteral("Apply"));
    ASSERT_NE(apply, nullptr);
    ASSERT_TRUE(until([=] { return apply->isEnabled(); }));
    d->findChildren<QSpinBox*>().last()->stepUp();
    apply->click();
    ASSERT_TRUE(until([=] { return apply->isEnabled(); }));
    bool found = false;
    for (auto* text : d->findChildren<QLabel*>())
        found |= text->text().contains(QStringLiteral("Automatic sync is off; time was not changed"));
    EXPECT_TRUE(found);
}
TEST_F(SystemDialogsTest, FailedSystemRestartRemainsVisibleAndCanBeCancelled) {
    mixxx::systemdialogs::power(nullptr);
    QCoreApplication::processEvents();
    auto* d = QApplication::activeWindow();
    ASSERT_NE(d, nullptr);
    auto* restart = findButton(d, QStringLiteral("Restart system"));
    ASSERT_NE(restart, nullptr);
    restart->click();
    QCoreApplication::processEvents();
    auto* confirmation = QApplication::activeWindow();
    ASSERT_NE(confirmation, d);
    auto* confirm = findButton(confirmation, QStringLiteral("Restart system"));
    ASSERT_NE(confirm, nullptr);
    confirm->click();
    ASSERT_TRUE(until([=] { return confirm->isEnabled(); }));
    EXPECT_TRUE(confirmation->isVisible());
    bool found = false;
    for (auto* text : confirmation->findChildren<QLabel*>())
        found |= text->text().contains(QStringLiteral("Request failed"));
    EXPECT_TRUE(found);
}
TEST_F(SystemDialogsTest, ClosingWhileReadingClockDoesNotRunCallbacksOnDeletedWidgets) {
    QFile fake(commands.filePath("timedatectl"));
    ASSERT_TRUE(fake.open(QIODevice::WriteOnly | QIODevice::Truncate));
    fake.write("#!/bin/sh\nexec /bin/sleep 2\n"); fake.close();
    mixxx::systemdialogs::clock(nullptr);
    QCoreApplication::processEvents();
    QPointer<QWidget> d = QApplication::activeWindow();
    ASSERT_NE(d, nullptr);
    auto* cancel = findButton(d, QStringLiteral("Cancel"));
    ASSERT_NE(cancel, nullptr);
    cancel->click();
    ASSERT_TRUE(until([&] { return d.isNull(); }));
}
TEST_F(SystemDialogsTest, ConfirmedApplicationRestartExitsEvenWhileConfirmationIsBusy) {
    mixxx::systemdialogs::power(nullptr);
    QCoreApplication::processEvents();
    auto* menu = QApplication::activeWindow();
    ASSERT_NE(menu, nullptr);
    auto* restart = findButton(menu, QStringLiteral("Restart BiteDJ"));
    ASSERT_NE(restart, nullptr);
    restart->click();
    QCoreApplication::processEvents();
    auto* confirmation = QApplication::activeWindow();
    ASSERT_NE(confirmation, menu);
    auto* confirm = findButton(confirmation, QStringLiteral("Restart BiteDJ"));
    ASSERT_NE(confirm, nullptr);
    QTimer watchdog;
    watchdog.setSingleShot(true);
    QObject::connect(&watchdog, &QTimer::timeout, [] { QCoreApplication::exit(99); });
    watchdog.start(1000);
    QTimer::singleShot(0, confirm, &QPushButton::click);
    EXPECT_EQ(QCoreApplication::exec(), 0);
    watchdog.stop();
    EXPECT_TRUE(qApp->property("bitedjRestart").toBool());
    qApp->setProperty("bitedjRestart", false);
}

TEST_F(SystemDialogsTest, OverclockTouchSaveReopenDefaultsAndRestartConfirmation) {
    const QString path = commands.filePath(QStringLiteral("config.txt"));
    QFile file(path);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    file.write(bootConfig); file.close();
    auto reader = [path] {
        QFile boot(path); boot.open(QIODevice::ReadOnly);
        auto settings = parse(boot.readAll(), QStringLiteral("Raspberry Pi 4 Model B (simulated)"));
        settings.path = path;
        return settings;
    };
    auto capture = [](const QString& name) {
        const QString directory = qEnvironmentVariable("BITEDJ_UI_CAPTURE_DIR");
        if (directory.isEmpty()) return;
        QThread::msleep(150);
        QCoreApplication::processEvents();
        QDir().mkpath(directory);
        // Optional real-display evidence from the owned GUI test container.
        EXPECT_EQ(QProcess::execute(QStringLiteral("/usr/bin/scrot"),
                          {QStringLiteral("-o"), directory + '/' + name + QStringLiteral(".png")}), 0);
    };
    mixxx::systemdialogs::overclock(nullptr, reader);
    ASSERT_TRUE(until([] { return QApplication::activeWindow() != nullptr; }));
    QPointer<QWidget> d = QApplication::activeWindow();
    ASSERT_NE(d, nullptr);
    auto* saveButton = findButton(d, QStringLiteral("Save for next restart"));
    ASSERT_NE(saveButton, nullptr);
    ASSERT_TRUE(until([=] { return saveButton->isEnabled(); }));
    auto fields = d->findChildren<QSpinBox*>();
    ASSERT_EQ(fields.size(), 3);
    EXPECT_EQ(fields[0]->value(), 2000);
    EXPECT_EQ(fields[1]->value(), 750);
    EXPECT_EQ(fields[2]->value(), 6);
    capture(QStringLiteral("overclock-simulated-initial"));
    findButton(fields[0]->parentWidget(), QStringLiteral("+"))->click();
    findButton(fields[1]->parentWidget(), QStringLiteral("−"))->click();
    findButton(fields[2]->parentWidget(), QStringLiteral("−"))->click();
    EXPECT_EQ(fields[0]->value(), 2100);
    EXPECT_EQ(fields[1]->value(), 700);
    EXPECT_EQ(fields[2]->value(), 5);
    saveButton->click();
    EXPECT_FALSE(fields[0]->isEnabled());
    ASSERT_TRUE(until([=] { return saveButton->isEnabled(); }));
    ASSERT_TRUE(file.open(QIODevice::ReadOnly));
    EXPECT_EQ(file.readAll(), render(bootConfig, 2100, 700, 5)); file.close();
    QFile backup(path + QStringLiteral(".bitedj-backup"));
    ASSERT_TRUE(backup.open(QIODevice::ReadOnly));
    EXPECT_EQ(backup.readAll(), bootConfig);
    capture(QStringLiteral("overclock-simulated-saved"));
    findButton(d, QStringLiteral("Back"))->click();
    ASSERT_TRUE(until([&] { return d.isNull(); }));

    mixxx::systemdialogs::overclock(nullptr, reader);
    ASSERT_TRUE(until([] { return QApplication::activeWindow() != nullptr; }));
    d = QApplication::activeWindow();
    ASSERT_NE(d, nullptr);
    saveButton = findButton(d, QStringLiteral("Save for next restart"));
    ASSERT_TRUE(until([=] { return saveButton->isEnabled(); }));
    fields = d->findChildren<QSpinBox*>();
    EXPECT_EQ(fields[0]->value(), 2100);
    EXPECT_EQ(fields[1]->value(), 700);
    EXPECT_EQ(fields[2]->value(), 5);
    capture(QStringLiteral("overclock-simulated-reopened"));
    findButton(d, QStringLiteral("Firmware defaults"))->click();
    for (auto* field : fields) EXPECT_EQ(field->value(), 0);
    saveButton->click();
    ASSERT_TRUE(until([=] { return saveButton->isEnabled(); }));
    ASSERT_TRUE(file.open(QIODevice::ReadOnly));
    const QByteArray defaults = file.readAll(); file.close();
    EXPECT_FALSE(defaults.contains("arm_freq="));
    EXPECT_FALSE(defaults.contains("gpu_freq="));
    EXPECT_FALSE(defaults.contains("over_voltage="));
    EXPECT_TRUE(defaults.contains("dtoverlay=vc4-kms-v3d"));
    capture(QStringLiteral("overclock-simulated-defaults"));
    auto* restart = findButton(d, QStringLiteral("Restart system…"));
    ASSERT_TRUE(restart->isEnabled());
    restart->click();
    ASSERT_TRUE(until([=] {
        return QApplication::activeWindow() && QApplication::activeWindow() != d.data();
    }));
    auto* confirmation = QApplication::activeWindow();
    ASSERT_NE(confirmation, d.data());
    ASSERT_NE(findButton(confirmation, QStringLiteral("Restart system")), nullptr);
    capture(QStringLiteral("overclock-simulated-restart-confirmation"));
    findButton(confirmation, QStringLiteral("Cancel"))->click();
    EXPECT_FALSE(QFile::exists(commands.filePath("commands.log")));
}

TEST_F(SystemDialogsTest, OverclockCancelAndStaleFileKeepUserConfiguration) {
    const QString path = commands.filePath(QStringLiteral("config.txt"));
    QFile file(path);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly)); file.write(bootConfig); file.close();
    auto reader = [path] {
        QFile boot(path); boot.open(QIODevice::ReadOnly);
        auto settings = parse(boot.readAll(), model); settings.path = path; return settings;
    };
    mixxx::systemdialogs::overclock(nullptr, reader);
    ASSERT_TRUE(until([] { return QApplication::activeWindow() != nullptr; }));
    QPointer<QWidget> d = QApplication::activeWindow();
    ASSERT_NE(d, nullptr);
    auto* saveButton = findButton(d, QStringLiteral("Save for next restart"));
    ASSERT_TRUE(until([=] { return saveButton->isEnabled(); }));
    d->findChildren<QSpinBox*>()[0]->setValue(2100);
    findButton(d, QStringLiteral("Back"))->click();
    ASSERT_TRUE(until([&] { return d.isNull(); }));
    ASSERT_TRUE(file.open(QIODevice::ReadOnly)); EXPECT_EQ(file.readAll(), bootConfig); file.close();

    mixxx::systemdialogs::overclock(nullptr, reader);
    ASSERT_TRUE(until([] { return QApplication::activeWindow() != nullptr; })); d = QApplication::activeWindow();
    ASSERT_NE(d, nullptr);
    saveButton = findButton(d, QStringLiteral("Save for next restart"));
    ASSERT_TRUE(until([=] { return saveButton->isEnabled(); }));
    const QByteArray external = bootConfig + "# changed by another editor\n";
    ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Truncate)); file.write(external); file.close();
    saveButton->click();
    ASSERT_TRUE(until([=] { return saveButton->isEnabled(); }));
    ASSERT_TRUE(file.open(QIODevice::ReadOnly)); EXPECT_EQ(file.readAll(), external);
    bool found = false;
    for (auto* text : d->findChildren<QLabel*>())
        found |= text->text().contains(QStringLiteral("Boot configuration changed"));
    EXPECT_TRUE(found);
}

TEST_F(SystemDialogsTest, TimezoneChangePreservesInstantAndAutomaticSync) {
    mixxx::systemdialogs::clock(nullptr);
    QCoreApplication::processEvents();
    auto* d = QApplication::activeWindow();
    ASSERT_NE(d, nullptr);
    auto* apply = findButton(d, QStringLiteral("Apply"));
    ASSERT_TRUE(until([=] { return apply->isEnabled(); }));
    auto boxes = d->findChildren<QComboBox*>();
    ASSERT_EQ(boxes.size(), 2);
    boxes[0]->setCurrentText(QStringLiteral("Brazil"));
    const int city = boxes[1]->findData(QByteArray("America/Sao_Paulo"));
    ASSERT_GE(city, 0); boxes[1]->setCurrentIndex(city);
    const QString preview = d->findChild<QLabel*>(QStringLiteral("ClockPreview"))->text();
    EXPECT_TRUE(preview.contains(QDateTime::currentDateTimeUtc().toTimeZone(QTimeZone("America/Sao_Paulo")).toString("HH:mm")));
    apply->click();
    ASSERT_TRUE(until([=] { return apply->isEnabled(); }));
    QFile log(commands.filePath("commands.log")); ASSERT_TRUE(log.open(QIODevice::ReadOnly));
    const auto calls = log.readAll();
    EXPECT_TRUE(calls.contains("set-timezone America/Sao_Paulo\nset-ntp true"));
    EXPECT_FALSE(calls.contains("set-time "));
    EXPECT_EQ(qApp->property("bitedjTimeZone").toByteArray(), QByteArray("America/Sao_Paulo"));
    qApp->setProperty("bitedjTimeZone", QVariant());
}
TEST_F(SystemDialogsTest, CalendarSelectsLeapDayAndManualTime) {
    mixxx::systemdialogs::clock(nullptr);
    QCoreApplication::processEvents();
    auto* d = QApplication::activeWindow(); ASSERT_NE(d, nullptr);
    auto* apply = findButton(d, QStringLiteral("Apply"));
    ASSERT_TRUE(until([=] { return apply->isEnabled(); }));
    d->findChild<QCheckBox*>()->setChecked(false);
    d->findChild<QPushButton*>(QStringLiteral("ClockDateButton"))->click();
    QCoreApplication::processEvents();
    auto* picker = QApplication::activeWindow(); ASSERT_NE(picker, d);
    auto* calendar = picker->findChild<QCalendarWidget*>(); ASSERT_NE(calendar, nullptr);
    calendar->setSelectedDate(QDate(2028, 2, 29));
    findButton(picker, QStringLiteral("Use date"))->click();
    auto fields = d->findChildren<QSpinBox*>();
    fields[0]->setValue(23); fields[1]->setValue(45);
    apply->click(); ASSERT_TRUE(until([=] { return apply->isEnabled(); }));
    QFile log(commands.filePath("commands.log")); ASSERT_TRUE(log.open(QIODevice::ReadOnly));
    EXPECT_TRUE(log.readAll().contains("set-ntp false\nset-time 2028-02-29 23:45:00"));
}
TEST_F(SystemDialogsTest, FailedTimezoneStopsBeforeChangingSyncOrTime) {
    QFile fake(commands.filePath("timedatectl"));
    ASSERT_TRUE(fake.open(QIODevice::Append));
    fake.write("if [ \"$1\" = set-timezone ]; then printf 'permission denied\\n' >&2; exit 1; fi\n"); fake.close();
    mixxx::systemdialogs::clock(nullptr);
    QCoreApplication::processEvents();
    auto* d = QApplication::activeWindow(); ASSERT_NE(d, nullptr);
    auto* apply = findButton(d, QStringLiteral("Apply"));
    ASSERT_TRUE(until([=] { return apply->isEnabled(); }));
    auto boxes = d->findChildren<QComboBox*>();
    boxes[0]->setCurrentText(QStringLiteral("Japan"));
    boxes[1]->setCurrentIndex(boxes[1]->findData(QByteArray("Asia/Tokyo")));
    apply->click(); ASSERT_TRUE(until([=] { return apply->isEnabled(); }));
    QFile log(commands.filePath("commands.log")); ASSERT_TRUE(log.open(QIODevice::ReadOnly));
    const auto calls = log.readAll();
    EXPECT_TRUE(calls.contains("set-timezone Asia/Tokyo"));
    EXPECT_FALSE(calls.contains("set-ntp"));
    EXPECT_FALSE(calls.contains("set-time "));
}

TEST(BootSettingsTest, PrivilegedRequestValidatesHashValuesAndKeepsBackup) {
    QTemporaryDir directory;
    auto current = parse(bootConfig, model);
    current.path = directory.filePath(QStringLiteral("config.txt"));
    QFile file(current.path); ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    file.write(bootConfig); file.close();
    const QString hash = QString::fromLatin1(QCryptographicHash::hash(bootConfig, QCryptographicHash::Sha256).toHex());
    EXPECT_FALSE(applyRequest(current, {QString(64, '0'), "2100", "700", "5"}).isEmpty());
    EXPECT_FALSE(applyRequest(current, {hash, "2100", "700", "5", "/etc/arbitrary"}).isEmpty());
    EXPECT_FALSE(applyRequest(current, {hash, "no", "700", "5"}).isEmpty());
    EXPECT_FALSE(applyRequest(current, {hash, "9900", "700", "5"}).isEmpty());
    EXPECT_FALSE(QFile::exists(current.path + ".bitedj-backup"));
    ASSERT_TRUE(applyRequest(current, {hash, "2100", "700", "5"}).isEmpty());
    ASSERT_TRUE(file.open(QIODevice::ReadOnly));
    EXPECT_EQ(file.readAll(), render(bootConfig, 2100, 700, 5)); file.close();
    QFile backup(current.path + ".bitedj-backup"); ASSERT_TRUE(backup.open(QIODevice::ReadOnly));
    EXPECT_EQ(backup.readAll(), bootConfig);
    EXPECT_FALSE(applyRequest(current, {hash, "2000", "750", "6"}).isEmpty());
}

TEST_F(SystemDialogsTest, NativeDialogIsIndependentAndClosesWithItsSkinOwner) {
    auto* owner = new QWidget;
    owner->setStyleSheet(QStringLiteral("QDialog { background: #123456; }"));
    owner->show();
    mixxx::systemdialogs::clock(owner);
    QCoreApplication::processEvents();
    auto* d = QApplication::activeWindow();
    ASSERT_NE(d, nullptr);
    ASSERT_NE(d, owner);
    EXPECT_EQ(d->parentWidget(), nullptr);
    EXPECT_FALSE(owner->isVisible());
    EXPECT_TRUE(d->isFullScreen());
    EXPECT_TRUE(d->styleSheet().contains(QStringLiteral("#123456")));
    QPointer<QWidget> tracked(d);
    delete owner;
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    EXPECT_TRUE(tracked.isNull());
}

TEST_F(SystemDialogsTest, CountryPickerOffersShortMainCityLists) {
    mixxx::systemdialogs::clock(nullptr);
    QCoreApplication::processEvents();
    auto* d = QApplication::activeWindow(); ASSERT_NE(d, nullptr);
    auto boxes = d->findChildren<QComboBox*>(); ASSERT_EQ(boxes.size(), 2);
    EXPECT_EQ(boxes[0]->accessibleName(), QStringLiteral("Country"));
    boxes[0]->setCurrentText(QStringLiteral("Brazil"));
    EXPECT_EQ(boxes[1]->count(), 5);
    EXPECT_GE(boxes[1]->findData(QByteArray("America/Sao_Paulo")), 0);
    EXPECT_EQ(boxes[1]->findData(QByteArray("America/Eirunepe")), -1);
    boxes[0]->setCurrentText(QStringLiteral("United States"));
    EXPECT_EQ(boxes[1]->count(), 7);
    boxes[0]->setCurrentText(QStringLiteral("Japan"));
    EXPECT_EQ(boxes[1]->count(), 1);
    EXPECT_EQ(boxes[1]->currentData().toByteArray(), QByteArray("Asia/Tokyo"));
}

TEST_F(SystemDialogsTest, SystemDialogRestoresOwnerAfterCancel) {
    QWidget owner;
    owner.showFullScreen();
    mixxx::systemdialogs::power(&owner);
    QCoreApplication::processEvents();
    auto* d = QApplication::activeWindow(); ASSERT_NE(d, &owner);
    ASSERT_FALSE(owner.isVisible());
    findButton(d, QStringLiteral("Back"))->click();
    EXPECT_TRUE(owner.isVisible());
    EXPECT_TRUE(owner.isFullScreen());
}

TEST_F(SystemDialogsTest, MainWindowActionsInheritCentralSkinTheme) {
    QMainWindow owner;
    auto* skin = new QWidget;
    skin->setStyleSheet(QStringLiteral("QDialog#SystemDialog { color: #abcdef; }"));
    owner.setCentralWidget(skin);
    owner.showFullScreen();
    mixxx::systemdialogs::power(&owner);
    QCoreApplication::processEvents();
    auto* d = QApplication::activeWindow(); ASSERT_NE(d, nullptr);
    ASSERT_NE(d, &owner);
    EXPECT_TRUE(d->styleSheet().contains(QStringLiteral("#abcdef")));
    findButton(d, QStringLiteral("Back"))->click();
    EXPECT_TRUE(owner.isVisible());
}
