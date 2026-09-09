// Copyright (C) 2026 Custom Bite DJ contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QByteArray>
#include <QString>
#include <QStringList>

namespace mixxx::bootsettings {
struct Settings {
    QString path;
    QString model;
    QByteArray original;
    int cpu = 0;
    int gpu = 0;
    int voltage = 0;
    QString error;
};
Settings parse(const QByteArray& contents, const QString& model);
QByteArray render(const QByteArray& contents, int cpu, int gpu, int voltage);
// Filesystem operations run on a worker thread.
Settings read();
QString save(const Settings& original, int cpu, int gpu, int voltage);
// The headless privileged entry point reads the actual boot file itself; requests
// contain only its expected SHA-256 and three numeric values, never a filename.
QString applyRequest(const Settings& current, const QStringList& request);
} // namespace mixxx::bootsettings
