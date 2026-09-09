#include "util/systemtelemetry.h"

#include <QDir>
#include <QFile>

namespace mixxx::systemtelemetry {
std::optional<CpuTicks> parseCpuTicks(const QByteArray& line) {
    const auto fields = line.simplified().split(' ');
    if (fields.size() < 5 || fields[0] != "cpu") {
        return std::nullopt;
    }
    CpuTicks ticks{0, 0};
    // guest/guest_nice are already included in user/nice. Count only the
    // first eight counters; idle includes iowait, matching Linux CPU usage.
    for (int i = 1; i < fields.size() && i <= 8; ++i) {
        bool ok = false;
        const auto value = fields[i].toULongLong(&ok);
        if (!ok) {
            return std::nullopt;
        }
        ticks.total += value;
        if (i == 4 || i == 5) {
            ticks.idle += value;
        }
    }
    return ticks;
}

std::optional<double> cpuUsage(const CpuTicks& previous, const CpuTicks& current) {
    if (current.total <= previous.total || current.idle < previous.idle) {
        return std::nullopt;
    }
    const auto elapsed = current.total - previous.total;
    const auto idle = current.idle - previous.idle;
    if (idle > elapsed) {
        return std::nullopt;
    }
    return 100.0 * static_cast<double>(elapsed - idle) / elapsed;
}

std::optional<double> parseTemperature(const QByteArray& value) {
    bool ok = false;
    // sysfs reports integer millidegrees. Integer parsing also rejects NaN
    // under the engine build's fast-math flags (isfinite is optimized away).
    const auto millidegrees = value.trimmed().toLongLong(&ok);
    if (!ok || millidegrees < -40000 || millidegrees > 150000) {
        return std::nullopt;
    }
    return millidegrees / 1000.0;
}

Snapshot readSnapshot() {
    Snapshot result;
#ifdef Q_OS_LINUX
    QFile cpu(QStringLiteral("/proc/stat"));
    if (cpu.open(QIODevice::ReadOnly)) {
        result.cpu = parseCpuTicks(cpu.readLine());
    }
    const QDir thermal(QStringLiteral("/sys/class/thermal"));
    for (const auto& zone : thermal.entryList({QStringLiteral("thermal_zone*")}, QDir::Dirs)) {
        const QString base = thermal.filePath(zone) + QLatin1Char('/');
        QFile typeFile(base + QStringLiteral("type"));
        if (!typeFile.open(QIODevice::ReadOnly)) {
            continue;
        }
        const auto type = typeFile.readAll().trimmed();
        // Pi kernels expose cpu-thermal or cpu_thermal. Do not silently
        // substitute a GPU, disk or ambient sensor on another machine.
        if (type != "cpu-thermal" && type != "cpu_thermal" && type != "soc_thermal") {
            continue;
        }
        QFile temperature(base + QStringLiteral("temp"));
        if (temperature.open(QIODevice::ReadOnly)) {
            result.temperature = parseTemperature(temperature.readAll());
        }
        if (result.temperature) {
            break;
        }
    }
#endif
    return result;
}
} // namespace mixxx::systemtelemetry
