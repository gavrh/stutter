#pragma once

#include <QDateTime>
#include <QString>

namespace stutter {

struct Conversation {
    QString id;
    QString binaryId;
    QString title;
    QString summary;
    qint64 inputTokens = 0;
    qint64 outputTokens = 0;
    QDateTime createdAt;
    QDateTime updatedAt;

    bool isValid() const { return !id.isEmpty(); }
};

}
