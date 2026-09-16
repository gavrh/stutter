#pragma once

#include <QString>
#include <QUrl>
#include <QVariantMap>

namespace stutter {

struct ProviderConfig {
    QString providerId;
    QString modelId;
    QUrl endpoint;
    QString effort;
    int maxOutputTokens = 4096;
    double temperature = -1.0;
    QVariantMap options;

    bool isValid() const {
        return !providerId.isEmpty() && !modelId.isEmpty();
    }
};

}
