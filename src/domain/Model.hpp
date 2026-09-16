#pragma once

#include <QFlags>
#include <QString>
#include <QStringList>
#include <QtGlobal>

namespace stutter {

enum class ModelCapability : quint32 {
    None = 0,
    Text = 1 << 0,
    Vision = 1 << 1,
    Reasoning = 1 << 2,
    Streaming = 1 << 3,
    Tools = 1 << 4
};
Q_DECLARE_FLAGS(ModelCapabilities, ModelCapability)

struct Model {
    QString providerId;
    QString id;
    QString name;
    qint64 contextWindow = 0;
    qint64 maxOutputTokens = 0;
    ModelCapabilities capabilities;
    QStringList supportedEfforts;
    QString defaultEffort;

    bool supports(ModelCapability capability) const {
        return capabilities.testFlag(capability);
    }
};

}

Q_DECLARE_OPERATORS_FOR_FLAGS(stutter::ModelCapabilities)
