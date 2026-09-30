#pragma once

#include <domain/BinaryIdentity.hpp>

#include <QHash>
#include <QMutex>
#include <QPair>
#include <QSet>
#include <QString>
#include <QVector>

namespace stutter {

class Database;

class BinaryRepository {
public:
    explicit BinaryRepository(Database& database);

    stutter::BinaryIdentity resolve(const stutter::BinaryIdentity& identity);
    stutter::BinaryIdentity identityFromPath(const QString& path);

    QVector<stutter::BinaryIdentity> all();
    bool remove(const QString& id);

private:
    Database& database_;
    QHash<QString, QString> hashes_;
    QHash<QString, QPair<qint64, qint64>> signatures_;
    QSet<QString> pending_;
    QMutex cacheMutex_;
};

}
