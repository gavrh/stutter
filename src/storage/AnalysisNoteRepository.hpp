#pragma once

#include <domain/AnalysisNote.hpp>

#include <QString>
#include <QVector>

namespace stutter {

class Database;

class AnalysisNoteRepository {
public:
    explicit AnalysisNoteRepository(Database& database);

    QVector<stutter::AnalysisNote> forBinary(const QString& binaryId);
    QVector<stutter::AnalysisNote> forBinaryAtAddress(const QString& binaryId, quint64 address);

    bool save(const stutter::AnalysisNote& note);
    bool remove(const QString& noteId);

private:
    Database& database_;
};

}
