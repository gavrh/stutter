#pragma once

#include <QString>

namespace stutter {

class Database;

class SettingsRepository {
public:
    explicit SettingsRepository(Database& database);

    QString value(const QString& key, const QString& fallback = {});
    bool setValue(const QString& key, const QString& value);
    bool remove(const QString& key);

    QString apiKey(const QString& providerId);
    bool setApiKey(const QString& providerId, const QString& apiKey);
    bool removeApiKey(const QString& providerId);

private:
    Database& database_;
};

}
