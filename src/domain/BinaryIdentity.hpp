#pragma once

#include <QString>

namespace stutter {

struct BinaryIdentity {
    QString sha256;
    QString path;
    QString name;
    QString version;

    bool isValid() const { return !sha256.isEmpty(); }
};

}
