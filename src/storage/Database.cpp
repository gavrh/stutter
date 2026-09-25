#include <storage/Database.hpp>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QStandardPaths>

#include <sqlite3.h>

namespace stutter {

static QStringList migrationScripts() {
    QDir directory(QStringLiteral(":/stutter/migrations"));
    if (!directory.exists()) {
        return {};
    }
    return directory.entryList({QStringLiteral("*.sql")}, QDir::Files, QDir::Name);
}

Database::~Database() {
    close();
}

bool Database::open(const QString& path) {
    close();

    path_ = path;
    const bool inMemory = path.isEmpty() || path == QStringLiteral(":memory:");
    if (!inMemory) {
        QDir().mkpath(QFileInfo(path).absolutePath());
    }

    const QByteArray pathBytes = path.toUtf8();
    if (sqlite3_open(inMemory ? ":memory:" : pathBytes.constData(), &database_) != SQLITE_OK) {
        lastError_ = database_
            ? QString::fromUtf8(sqlite3_errmsg(database_))
            : QStringLiteral("Unable to open database");
        close();
        return false;
    }

    sqlite3_busy_timeout(database_, 5000);
    execute(QStringLiteral("PRAGMA foreign_keys = ON"));
    execute(QStringLiteral("PRAGMA journal_mode = WAL"));
    return true;
}

void Database::close() {
    if (database_) {
        sqlite3_close(database_);
        database_ = nullptr;
    }
}

bool Database::execute(const QString& sql) {
    if (!database_) {
        lastError_ = QStringLiteral("Database is not open");
        return false;
    }

    char* error = nullptr;
    const int result = sqlite3_exec(
        database_,
        sql.toUtf8().constData(),
        nullptr,
        nullptr,
        &error
    );
    if (result != SQLITE_OK) {
        lastError_ = QString::fromUtf8(error ? error : "SQL execution failed");
        sqlite3_free(error);
        return false;
    }
    return true;
}

bool Database::transaction(const std::function<bool()>& work) {
    if (!execute(QStringLiteral("BEGIN"))) {
        return false;
    }
    if (!work()) {
        execute(QStringLiteral("ROLLBACK"));
        return false;
    }
    if (!execute(QStringLiteral("COMMIT"))) {
        execute(QStringLiteral("ROLLBACK"));
        return false;
    }
    return true;
}

bool Database::applyMigrations() {
    const QStringList scripts = migrationScripts();
    if (scripts.isEmpty()) {
        return true;
    }

    if (!execute(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS schema_migrations ("
        "version TEXT PRIMARY KEY, applied_at TEXT NOT NULL)"
    ))) {
        return false;
    }

    QSet<QString> applied;
    {
        Statement statement(*this, QStringLiteral("SELECT version FROM schema_migrations"));
        while (statement.next()) {
            applied.insert(statement.text(0));
        }
    }

    for (const QString& file : scripts) {
        const QString version = file.section(QLatin1Char('.'), 0, 0);
        if (applied.contains(version)) {
            continue;
        }

        QFile script(QStringLiteral(":/stutter/migrations/%1").arg(file));
        if (!script.open(QIODevice::ReadOnly | QIODevice::Text)) {
            lastError_ = QStringLiteral("Unable to read migration: %1").arg(file);
            return false;
        }
        const QByteArray sql = script.readAll();

        const bool ok = transaction([&] {
            char* error = nullptr;
            if (sqlite3_exec(database_, sql.constData(), nullptr, nullptr, &error) != SQLITE_OK) {
                lastError_ = QString::fromUtf8(error ? error : "Migration failed");
                sqlite3_free(error);
                return false;
            }
            Statement record(
                *this,
                QStringLiteral("INSERT INTO schema_migrations(version, applied_at) VALUES (?1, ?2)")
            );
            record.bind(1, version);
            record.bind(2, QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
            record.next();
            return record.error().isEmpty();
        });
        if (!ok) {
            return false;
        }
    }
    return true;
}

QString Database::defaultDatabasePath() {
    const QString base = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    return QDir(base).filePath(QStringLiteral("stutter/stutter.db"));
}

Statement::Statement(Database& database, const QString& sql) {
    if (!database.isOpen()) {
        error_ = QStringLiteral("Database is not open");
        return;
    }
    if (sqlite3_prepare_v2(
            database.handle(),
            sql.toUtf8().constData(),
            -1,
            &statement_,
            nullptr
        ) != SQLITE_OK) {
        error_ = QString::fromUtf8(sqlite3_errmsg(database.handle()));
        statement_ = nullptr;
    }
}

Statement::~Statement() {
    if (statement_) {
        sqlite3_finalize(statement_);
    }
}

bool Statement::bind(int index, const QString& value) {
    if (!statement_) {
        return false;
    }
    const QByteArray bytes = value.toUtf8();
    return sqlite3_bind_text(
        statement_,
        index,
        bytes.constData(),
        bytes.size(),
        SQLITE_TRANSIENT
    ) == SQLITE_OK;
}

bool Statement::bind(int index, qint64 value) {
    return statement_ && sqlite3_bind_int64(statement_, index, value) == SQLITE_OK;
}

bool Statement::bind(int index, double value) {
    return statement_ && sqlite3_bind_double(statement_, index, value) == SQLITE_OK;
}

bool Statement::bindNull(int index) {
    return statement_ && sqlite3_bind_null(statement_, index) == SQLITE_OK;
}

bool Statement::next() {
    if (!statement_) {
        return false;
    }
    const int result = sqlite3_step(statement_);
    if (result == SQLITE_ROW) {
        return true;
    }
    if (result == SQLITE_DONE) {
        return false;
    }
    error_ = QString::fromUtf8(sqlite3_errmsg(sqlite3_db_handle(statement_)));
    return false;
}

void Statement::reset() {
    if (statement_) {
        sqlite3_reset(statement_);
        sqlite3_clear_bindings(statement_);
    }
}

QString Statement::text(int column) const {
    if (!statement_) {
        return {};
    }
    const unsigned char* value = sqlite3_column_text(statement_, column);
    return value ? QString::fromUtf8(reinterpret_cast<const char*>(value)) : QString();
}

qint64 Statement::integer(int column) const {
    return statement_ ? sqlite3_column_int64(statement_, column) : 0;
}

double Statement::number(int column) const {
    return statement_ ? sqlite3_column_double(statement_, column) : 0.0;
}

bool Statement::isNull(int column) const {
    return !statement_ || sqlite3_column_type(statement_, column) == SQLITE_NULL;
}

}
