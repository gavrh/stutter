#include <config/ModelCatalog.hpp>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>

namespace {
stutter::ModelCapability capabilityFromName(const QString& name) {
    if (name == QStringLiteral("text")) return stutter::ModelCapability::Text;
    if (name == QStringLiteral("vision")) return stutter::ModelCapability::Vision;
    if (name == QStringLiteral("reasoning")) return stutter::ModelCapability::Reasoning;
    if (name == QStringLiteral("streaming")) return stutter::ModelCapability::Streaming;
    if (name == QStringLiteral("tools")) return stutter::ModelCapability::Tools;
    return stutter::ModelCapability::None;
}

qint64 integerValue(const QJsonValue& value) {
    return static_cast<qint64>(value.toDouble());
}
}

ModelCatalog::ModelCatalog(const QString& resourcePath) {
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly)) {
        error_ = QStringLiteral("Unable to open model catalog: %1").arg(resourcePath);
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        error_ = parseError.errorString();
        return;
    }
    if (!document.isObject()) {
        error_ = QStringLiteral("Model catalog root must be an object");
        return;
    }

    const QJsonArray providers = document.object().value(QStringLiteral("providers")).toArray();
    for (const QJsonValue& providerValue : providers) {
        const QJsonObject provider = providerValue.toObject();
        const QString providerId = provider.value(QStringLiteral("id")).toString();
        defaultModels_.insert(
            providerId, provider.value(QStringLiteral("default_model")).toString()
        );
        defaultEndpoints_.insert(
            providerId,
            QUrl(provider.value(QStringLiteral("default_endpoint")).toString())
        );

        QVector<stutter::Model> models;
        const QJsonArray modelValues = provider.value(QStringLiteral("models")).toArray();
        for (const QJsonValue& modelValue : modelValues) {
            const QJsonObject modelObject = modelValue.toObject();
            stutter::Model model;
            model.providerId = providerId;
            model.id = modelObject.value(QStringLiteral("id")).toString();
            model.name = modelObject.value(QStringLiteral("name")).toString();
            model.contextWindow = integerValue(modelObject.value(QStringLiteral("context_window")));
            model.maxOutputTokens = integerValue(
                modelObject.value(QStringLiteral("max_output_tokens"))
            );
            model.defaultEffort = modelObject.value(QStringLiteral("default_effort")).toString();
            for (const QJsonValue& effort : modelObject.value(QStringLiteral("efforts")).toArray()) {
                model.supportedEfforts.append(effort.toString());
            }
            for (const QJsonValue& capability :
                 modelObject.value(QStringLiteral("capabilities")).toArray()) {
                model.capabilities |= capabilityFromName(capability.toString());
            }
            models.append(model);
        }
        models_.insert(providerId, models);
    }
}

QStringList ModelCatalog::providerIds() const {
    return models_.keys();
}

QVector<stutter::Model> ModelCatalog::modelsForProvider(const QString& providerId) const {
    return models_.value(providerId);
}

QString ModelCatalog::defaultModel(const QString& providerId) const {
    return defaultModels_.value(providerId);
}

QUrl ModelCatalog::defaultEndpoint(const QString& providerId) const {
    return defaultEndpoints_.value(providerId);
}
