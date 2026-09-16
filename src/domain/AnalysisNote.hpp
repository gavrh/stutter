#pragma once

#include <QDateTime>
#include <QString>
#include <QtGlobal>

namespace stutter {

struct AnalysisNote {
    QString id;
    QString binaryId;
    quint64 address = 0;
    QString category;
    QString title;
    QString content;
    double confidence = 0.0;
    QDateTime createdAt;
    QDateTime updatedAt;
};

}
