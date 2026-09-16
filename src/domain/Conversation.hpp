#pragma once

#include <QDateTime>
#include <QString>

namespace stutter {

struct Conversation {
    QString id;
    QString binaryId;
    QString title;
    QString summary;
    QDateTime createdAt;
    QDateTime updatedAt;

    bool isValid() const { return !id.isEmpty(); }
};

}
