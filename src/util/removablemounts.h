#pragma once

#include <QList>
#include <QStringList>

namespace mixxx {

struct RemovableMount {
    QString device;
    QString mountPoint;
};

// Linux mountinfo is kernel metadata. Unlike QStorageInfo/statfs, reading it
// cannot wait for a failing USB disk to answer a filesystem request.
inline QString decodeMountField(QString field) {
    // Decode backslash last: a literal "\\040" must not become a space.
    return field.replace(QStringLiteral("\\040"), QStringLiteral(" "))
            .replace(QStringLiteral("\\011"), QStringLiteral("\t"))
            .replace(QStringLiteral("\\012"), QStringLiteral("\n"))
            .replace(QStringLiteral("\\134"), QStringLiteral("\\"));
}

inline QList<RemovableMount> parseRemovableMounts(
        const QByteArray& mountInfo, const QStringList& roots) {
    QList<RemovableMount> mounts;
    for (const QByteArray& line : mountInfo.split('\n')) {
        const auto fields = line.split(' ');
        const auto separator = fields.indexOf("-");
        if (fields.size() < 10 || separator < 6 || separator + 2 >= fields.size()) {
            continue;
        }
        const QString path = decodeMountField(QString::fromUtf8(fields[4]));
        bool removable = false;
        for (const auto& root : roots) {
            if (path.startsWith(root + QLatin1Char('/'))) {
                removable = true;
                break;
            }
        }
        if (!removable) {
            continue;
        }
        const QString device = decodeMountField(QString::fromUtf8(fields[separator + 2]));
        // Stacked mounts expose the last mounted filesystem at a path.
        for (auto it = mounts.begin(); it != mounts.end();) {
            if (it->mountPoint == path) {
                it = mounts.erase(it);
            } else {
                ++it;
            }
        }
        mounts.append({device, path});
    }
    return mounts;
}

} // namespace mixxx
