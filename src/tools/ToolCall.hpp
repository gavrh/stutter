#pragma once

#include <QJsonObject>
#include <QString>

namespace stutter {

struct ToolCall {
    QString id;
    QString name;
    QJsonObject arguments;
    QString rawArguments;
};

}
