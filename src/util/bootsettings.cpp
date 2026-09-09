// Copyright (C) 2026 Custom Bite DJ contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#include "util/bootsettings.h"
#include <QFile>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QProcess>
#ifdef Q_OS_LINUX
#include <unistd.h>
#endif
#include <QFileInfo>
#include <QRegularExpression>
#include <QSaveFile>

namespace mixxx::bootsettings {
namespace {
const QRegularExpression key(QStringLiteral("^\\s*(arm_freq|gpu_freq|over_voltage)\\s*=\\s*(-?\\d+)\\s*(?:#.*)?$"));
const QByteArray marker("# Custom Bite DJ clock settings");
}
Settings parse(const QByteArray& contents, const QString& model) {
    Settings result;
    result.original = contents;
    result.model = model;
    const bool pi4 = model.startsWith(QStringLiteral("Raspberry Pi 4 Model"));
    const bool pi5 = model.startsWith(QStringLiteral("Raspberry Pi 5 Model"));
    if (!pi4 && !pi5) {
        result.error = QStringLiteral("Overclock settings are available on Raspberry Pi 4 and 5.");
        return result;
    }
    QString section;
    for (const auto& line : contents.split('\n')) {
        const QString text = QString::fromUtf8(line).trimmed();
        if (text.startsWith('#') || text.isEmpty()) continue;
        if (text.startsWith('[')) section = text;
        // Refuse configurations we cannot faithfully interpret or override.
        if (QRegularExpression(QStringLiteral("^(include\\s|(?:over_voltage_\\w+|arm_freq_min|gpu_freq_min|core_freq\\w*|v3d_freq\\w*|h264_freq\\w*|isp_freq\\w*|hevc_freq\\w*|sdram_freq\\w*)\\s*=|force_turbo\\s*=\\s*1)")).match(text).hasMatch()) {
            result.error = QStringLiteral("This boot configuration has external or advanced clock overrides. Edit it manually before using this editor.");
        }
        const auto match = key.match(text);
        if (!match.hasMatch()) {
            if (QRegularExpression(QStringLiteral("^(arm_freq|gpu_freq|over_voltage)\\s*=")).match(text).hasMatch())
                result.error = QStringLiteral("Invalid clock value in boot configuration.");
            continue;
        }
        if (!section.isEmpty() && section != QStringLiteral("[all]") &&
                section != (pi4 ? QStringLiteral("[pi4]") : QStringLiteral("[pi5]"))) {
            result.error = QStringLiteral("Clock overrides in conditional sections require manual configuration.");
            continue;
        }
        bool valid;
        const int value = match.captured(2).toInt(&valid);
        if (!valid) result.error = QStringLiteral("Invalid clock value in boot configuration.");
        if (match.captured(1) == QStringLiteral("arm_freq")) result.cpu = value;
        if (match.captured(1) == QStringLiteral("gpu_freq")) result.gpu = value;
        if (match.captured(1) == QStringLiteral("over_voltage")) result.voltage = value;
    }
    if ((result.cpu != 0 && (result.cpu < 1000 || result.cpu > (pi4 ? 2400 : 3000))) ||
            (result.gpu != 0 && (result.gpu < 400 || result.gpu > 1000)) ||
            result.voltage < 0 || result.voltage > 6) {
        result.error = QStringLiteral("Existing clock values are outside this editor's range; no changes were made.");
    }
    return result;
}
QByteArray render(const QByteArray& contents, int cpu, int gpu, int voltage) {
    QByteArray output;
    const auto base = contents.split('\n');
    for (const auto& line : base) {
        if (key.match(QString::fromUtf8(line)).hasMatch() || line == marker) continue;
        output += line + '\n';
    }
    while (output.endsWith('\n')) output.chop(1);
    if (!output.endsWith("[all]")) output += "\n[all]";
    output += '\n' + marker + '\n';
    if (cpu) output += "arm_freq=" + QByteArray::number(cpu) + '\n';
    if (gpu) output += "gpu_freq=" + QByteArray::number(gpu) + '\n';
    if (voltage) output += "over_voltage=" + QByteArray::number(voltage) + '\n';
    return output;
}
Settings read() {
    QFile model(QStringLiteral("/proc/device-tree/model"));
    const QString name = model.open(QIODevice::ReadOnly)
            ? QString::fromUtf8(model.readAll()).remove(QChar('\0')).trimmed() : QString();
    const QString path = QFileInfo::exists(QStringLiteral("/boot/firmware/config.txt"))
            ? QStringLiteral("/boot/firmware/config.txt") : QStringLiteral("/boot/config.txt");
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        Settings result;
        result.error = QStringLiteral("Overclock settings require a Raspberry Pi with a readable boot configuration.");
        return result;
    }
    Settings result = parse(file.readAll(), name);
    result.path = path;
    return result;
}
QString save(const Settings& original, int cpu, int gpu, int voltage) {
    if (!original.error.isEmpty()) return original.error;
    const QByteArray updated = render(original.original, cpu, gpu, voltage);
    const Settings validated = parse(updated, original.model);
    if (!validated.error.isEmpty()) return validated.error;
    QFile current(original.path);
    if (!current.open(QIODevice::ReadOnly) || current.readAll() != original.original)
        return QStringLiteral("Boot configuration changed or could not be read. Reopen the editor.");
    current.close();
#ifdef Q_OS_LINUX
    if (geteuid() != 0 && (original.path == QStringLiteral("/boot/firmware/config.txt") ||
                                 original.path == QStringLiteral("/boot/config.txt"))) {
        QProcess helper;
        helper.start(QStringLiteral("sudo"), {QStringLiteral("-n"), QCoreApplication::applicationFilePath(),
                QStringLiteral("--bitedj-apply-boot-settings"),
                QString::fromLatin1(QCryptographicHash::hash(original.original, QCryptographicHash::Sha256).toHex()),
                QString::number(cpu), QString::number(gpu), QString::number(voltage)});
        if (!helper.waitForStarted(5000)) return QStringLiteral("Could not start sudo to save boot settings.");
        if (!helper.waitForFinished(15000)) {
            helper.kill(); helper.waitForFinished(1000);
            return QStringLiteral("Saving timed out. Reopen the editor to check the saved values.");
        }
        if (helper.exitStatus() != QProcess::NormalExit || helper.exitCode() != 0)
            return QStringLiteral("Could not save boot settings with sudo: %1").arg(QString::fromUtf8(helper.readAllStandardError()).trimmed());
        return {};
    }
#endif
    // Keep the first pre-edit configuration for recovery; never overwrite it.
    const QString backup = original.path + QStringLiteral(".bitedj-backup");
    if (!QFileInfo::exists(backup) && !QFile::copy(original.path, backup))
        return QStringLiteral("Could not create a recovery backup. No settings changed.");
    QSaveFile file(original.path);
    if (!file.open(QIODevice::WriteOnly) || file.write(updated) != updated.size() || !file.commit())
        return QStringLiteral("Could not save boot settings. Check permissions and free space.");
    return {};
}
QString applyRequest(const Settings& current, const QStringList& request) {
    if (request.size() != 4 || !QRegularExpression(QStringLiteral("^[a-f0-9]{64}$")).match(request.value(0)).hasMatch())
        return QStringLiteral("Invalid boot-settings request.");
    if (!current.error.isEmpty()) return current.error;
    const auto hash = QCryptographicHash::hash(current.original, QCryptographicHash::Sha256).toHex();
    if (request[0].toLatin1() != hash)
        return QStringLiteral("Boot configuration changed. Reopen the editor.");
    bool c, g, v;
    const int cpu = request[1].toInt(&c), gpu = request[2].toInt(&g), voltage = request[3].toInt(&v);
    if (!c || !g || !v) return QStringLiteral("Invalid numeric boot settings.");
    return save(current, cpu, gpu, voltage);
}
} // namespace mixxx::bootsettings
