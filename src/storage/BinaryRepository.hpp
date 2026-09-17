#pragma once

#include <domain/BinaryIdentity.hpp>

#include <QString>

namespace stutter {

class Database;

class BinaryRepository {
public:
    explicit BinaryRepository(Database& database);

    stutter::BinaryIdentity resolve(const stutter::BinaryIdentity& identity);
    static stutter::BinaryIdentity identityFromPath(const QString& path);

private:
    Database& database_;
};

}
