#include <config/ModelCatalog.hpp>

#include <QFile>

#include <toml.hpp>

#include <sstream>
#include <string>
#include <vector>

namespace {
stutter::ModelCapability capabilityFromName(const std::string& name) {
    if (name == "text") return stutter::ModelCapability::Text;
    if (name == "vision") return stutter::ModelCapability::Vision;
    if (name == "reasoning") return stutter::ModelCapability::Reasoning;
    if (name == "streaming") return stutter::ModelCapability::Streaming;
    if (name == "tools") return stutter::ModelCapability::Tools;
    return stutter::ModelCapability::None;
}
}

ModelCatalog::ModelCatalog(const QString& resourcePath) {
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly)) {
        error_ = QStringLiteral("Unable to open model catalog: %1").arg(resourcePath);
        return;
    }

    try {
        std::istringstream input(file.readAll().toStdString());
        const toml::value root = toml::parse(input, resourcePath.toStdString());
        const auto providers = toml::find<std::vector<toml::value>>(root, "providers");
        for (const toml::value& providerValue : providers) {
            const QString providerId = QString::fromStdString(
                toml::find<std::string>(providerValue, "id")
            );
            defaultModels_.insert(providerId, QString::fromStdString(
                toml::find<std::string>(providerValue, "default_model")
            ));
            defaultEndpoints_.insert(providerId, QUrl(QString::fromStdString(
                toml::find<std::string>(providerValue, "default_endpoint")
            )));

            QVector<stutter::Model> models;
            const auto modelValues = toml::find<std::vector<toml::value>>(providerValue, "models");
            for (const toml::value& modelValue : modelValues) {
                stutter::Model model;
                model.providerId = providerId;
                model.id = QString::fromStdString(toml::find<std::string>(modelValue, "id"));
                model.name = QString::fromStdString(toml::find<std::string>(modelValue, "name"));
                model.contextWindow = toml::find<qint64>(modelValue, "context_window");
                model.maxOutputTokens = toml::find<qint64>(modelValue, "max_output_tokens");
                model.defaultEffort = QString::fromStdString(
                    toml::find_or<std::string>(modelValue, "default_effort", "")
                );
                for (const std::string& effort :
                     toml::find_or<std::vector<std::string>>(modelValue, "efforts", {})) {
                    model.supportedEfforts.append(QString::fromStdString(effort));
                }
                for (const std::string& capability :
                     toml::find<std::vector<std::string>>(modelValue, "capabilities")) {
                    model.capabilities |= capabilityFromName(capability);
                }
                models.append(model);
            }
            models_.insert(providerId, models);
        }
    } catch (const std::exception& exception) {
        models_.clear();
        defaultModels_.clear();
        defaultEndpoints_.clear();
        error_ = QString::fromUtf8(exception.what());
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
