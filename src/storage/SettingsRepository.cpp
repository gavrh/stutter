#include <storage/SettingsRepository.hpp>

#include <storage/Database.hpp>

namespace stutter {

SettingsRepository::SettingsRepository(Database& database) : database_(database) {}

QString SettingsRepository::value(const QString& key, const QString& fallback) {
    if (!database_.isOpen()) {
        return fallback;
    }
    Statement statement(database_, QStringLiteral("SELECT value FROM settings WHERE key = ?1"));
    statement.bind(1, key);
    if (statement.next()) {
        return statement.text(0);
    }
    return fallback;
}

bool SettingsRepository::setValue(const QString& key, const QString& value) {
    if (!database_.isOpen()) {
        return false;
    }
    Statement statement(
        database_,
        QStringLiteral(
            "INSERT INTO settings(key, value) VALUES (?1, ?2) "
            "ON CONFLICT(key) DO UPDATE SET value = excluded.value"
        )
    );
    statement.bind(1, key);
    statement.bind(2, value);
    statement.next();
    return statement.error().isEmpty();
}

bool SettingsRepository::remove(const QString& key) {
    if (!database_.isOpen()) {
        return false;
    }
    Statement statement(database_, QStringLiteral("DELETE FROM settings WHERE key = ?1"));
    statement.bind(1, key);
    statement.next();
    return statement.error().isEmpty();
}

QString SettingsRepository::apiKey(const QString& providerId) {
    if (!database_.isOpen()) {
        return {};
    }
    Statement statement(database_, QStringLiteral("SELECT api_key FROM api_keys WHERE provider = ?1"));
    statement.bind(1, providerId);
    if (statement.next()) {
        return statement.text(0);
    }
    return {};
}

bool SettingsRepository::setApiKey(const QString& providerId, const QString& apiKey) {
    if (!database_.isOpen()) {
        return false;
    }
    if (apiKey.isEmpty()) {
        return removeApiKey(providerId);
    }
    Statement statement(
        database_,
        QStringLiteral(
            "INSERT INTO api_keys(provider, api_key) VALUES (?1, ?2) "
            "ON CONFLICT(provider) DO UPDATE SET api_key = excluded.api_key"
        )
    );
    statement.bind(1, providerId);
    statement.bind(2, apiKey);
    statement.next();
    return statement.error().isEmpty();
}

bool SettingsRepository::removeApiKey(const QString& providerId) {
    if (!database_.isOpen()) {
        return false;
    }
    Statement statement(database_, QStringLiteral("DELETE FROM api_keys WHERE provider = ?1"));
    statement.bind(1, providerId);
    statement.next();
    return statement.error().isEmpty();
}

}
