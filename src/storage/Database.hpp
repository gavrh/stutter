#pragma once

#include <QString>
#include <QtGlobal>

#include <functional>

struct sqlite3;
struct sqlite3_stmt;

namespace stutter {

class Database {
public:
    Database() = default;
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    bool open(const QString& path);
    void close();

    bool isOpen() const { return database_ != nullptr; }
    QString path() const { return path_; }
    QString lastError() const { return lastError_; }

    bool execute(const QString& sql);
    bool transaction(const std::function<bool()>& work);

    bool applyMigrations();

    sqlite3* handle() const { return database_; }

    static QString defaultDatabasePath();

private:
    sqlite3* database_ = nullptr;
    QString path_;
    QString lastError_;
};

class Statement {
public:
    Statement(Database& database, const QString& sql);
    ~Statement();

    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;

    bool isValid() const { return statement_ != nullptr; }
    bool bind(int index, const QString& value);
    bool bind(int index, qint64 value);
    bool bind(int index, double value);
    bool bindNull(int index);

    bool next();
    void reset();

    QString text(int column) const;
    qint64 integer(int column) const;
    double number(int column) const;
    bool isNull(int column) const;

    QString error() const { return error_; }

private:
    sqlite3_stmt* statement_ = nullptr;
    QString error_;
};

}
