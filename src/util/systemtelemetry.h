#pragma once

#include <QByteArray>
#include <QString>
#include <optional>

namespace mixxx::systemtelemetry {
struct CpuTicks {
    quint64 total;
    quint64 idle;
};
struct Snapshot {
    std::optional<CpuTicks> cpu;
    std::optional<double> temperature;
};
std::optional<CpuTicks> parseCpuTicks(const QByteArray& line);
std::optional<double> cpuUsage(const CpuTicks& previous, const CpuTicks& current);
std::optional<double> parseTemperature(const QByteArray& value);
// Blocking OS reads; invoke on a worker, never on the GUI/audio thread.
Snapshot readSnapshot();
} // namespace mixxx::systemtelemetry
