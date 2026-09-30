#pragma once

#include <QDateTime>
#include <QString>

namespace stutter {

struct BinaryIdentity {
    QString id;
    QString sha256;
    QString path;
    QString name;
    QString version;
    QString projectPath;
    QDateTime createdAt;
    QDateTime updatedAt;
    QDateTime lastSeenAt;

    bool isValid() const { return !sha256.isEmpty(); }
    bool isResolvable() const { return !path.isEmpty() || !sha256.isEmpty(); }
};

}
