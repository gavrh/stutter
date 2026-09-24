#pragma once

#include <domain/Model.hpp>

#include <QHash>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVector>

class ModelCatalog {
public:
    explicit ModelCatalog(const QString& resourcePath = QStringLiteral(":/stutter/data/models.json"));

    bool isValid() const { return error_.isEmpty(); }
    QString error() const { return error_; }
    QStringList providerIds() const;
    QVector<stutter::Model> modelsForProvider(const QString& providerId) const;
    QString defaultModel(const QString& providerId) const;
    QUrl defaultEndpoint(const QString& providerId) const;

private:
    QHash<QString, QVector<stutter::Model>> models_;
    QHash<QString, QString> defaultModels_;
    QHash<QString, QUrl> defaultEndpoints_;
    QString error_;
};
