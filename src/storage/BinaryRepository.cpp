#include <storage/BinaryRepository.hpp>

#include <storage/Database.hpp>
#include <storage/StorageUtils.hpp>

#include <QCryptographicHash>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QUuid>

namespace stutter {

namespace {
const QString kSelectColumns =
    QStringLiteral("id, sha256, path, name, version, project_path, created_at, updated_at");

stutter::BinaryIdentity readBinary(Statement& statement) {
    stutter::BinaryIdentity identity;
    identity.id = statement.text(0);
    identity.sha256 = statement.text(1);
    identity.path = statement.text(2);
    identity.name = statement.text(3);
    identity.version = statement.text(4);
    identity.projectPath = statement.text(5);
    return identity;
}

QString computeSha256(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&file)) {
        return {};
    }
    return QString::fromLatin1(hash.result().toHex());
}
}

BinaryRepository::BinaryRepository(Database& database) : database_(database) {}

stutter::BinaryIdentity BinaryRepository::resolve(const stutter::BinaryIdentity& identity) {
    stutter::BinaryIdentity result = identity;
    if (!database_.isOpen() || !identity.isResolvable()) {
        return result;
    }

    const QString now = storage::toStorage(QDateTime::currentDateTimeUtc());
    if (!identity.path.isEmpty()) {
        Statement statement(
            database_,
            QStringLiteral("SELECT %1 FROM binaries WHERE path = ?1 LIMIT 1").arg(kSelectColumns)
        );
        statement.bind(1, identity.path);
        if (statement.next()) {
            const stutter::BinaryIdentity existing = readBinary(statement);
            result.id = existing.id;

            const bool changed = existing.sha256 != identity.sha256
                || existing.name != identity.name
                || existing.version != identity.version
                || existing.projectPath != identity.projectPath;
            if (changed) {
                Statement update(
                    database_,
                    QStringLiteral(
                        "UPDATE binaries SET sha256 = ?1, name = ?2, version = ?3, "
                        "project_path = ?4, updated_at = ?5 WHERE id = ?6"
                    )
                );
                update.bind(1, identity.sha256);
                update.bind(2, identity.name);
                update.bind(3, identity.version);
                update.bind(4, identity.projectPath);
                update.bind(5, now);
                update.bind(6, existing.id);
                update.next();
            }
            return result;
        }
    }

    if (!identity.sha256.isEmpty()) {
        Statement statement(
            database_,
            QStringLiteral("SELECT %1 FROM binaries WHERE sha256 = ?1 LIMIT 1").arg(kSelectColumns)
        );
        statement.bind(1, identity.sha256);
        if (statement.next()) {
            const stutter::BinaryIdentity existing = readBinary(statement);
            result.id = existing.id;

            Statement update(
                database_,
                QStringLiteral(
                    "UPDATE binaries SET path = ?1, name = ?2, version = ?3, "
                    "project_path = ?4, updated_at = ?5 WHERE id = ?6"
                )
            );
            update.bind(1, identity.path);
            update.bind(2, identity.name);
            update.bind(3, identity.version);
            update.bind(4, identity.projectPath);
            update.bind(5, now);
            update.bind(6, existing.id);
            update.next();
            return result;
        }
    }

    result.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    Statement insert(
        database_,
        QStringLiteral(
            "INSERT INTO binaries(id, sha256, path, name, version, project_path, created_at, updated_at) "
            "VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?7)"
        )
    );
    insert.bind(1, result.id);
    insert.bind(2, result.sha256);
    insert.bind(3, result.path);
    insert.bind(4, result.name);
    insert.bind(5, result.version);
    insert.bind(6, result.projectPath);
    insert.bind(7, now);
    insert.next();
    return result;
}

stutter::BinaryIdentity BinaryRepository::identityFromPath(const QString& path) {
    stutter::BinaryIdentity identity;
    identity.path = path;
    identity.name = QFileInfo(path).fileName();

    static QHash<QString, QString> cache;
    static QHash<QString, QPair<qint64, qint64>> signatures;

    const QFileInfo info(path);
    const QPair<qint64, qint64> signature {info.size(), info.lastModified().toMSecsSinceEpoch()};
    const auto cached = signatures.constFind(path);
    if (cached != signatures.constEnd() && cached.value() == signature) {
        identity.sha256 = cache.value(path);
        return identity;
    }

    identity.sha256 = computeSha256(path);
    signatures.insert(path, signature);
    cache.insert(path, identity.sha256);
    return identity;
}

}
