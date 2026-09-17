#pragma once

#include <QString>

namespace stutter {

struct BinaryIdentity {
    QString id;
    QString sha256;
    QString path;
    QString name;
    QString version;
    QString projectPath;

    bool isValid() const { return !sha256.isEmpty(); }
    bool isResolvable() const { return !path.isEmpty() || !sha256.isEmpty(); }
};

}
