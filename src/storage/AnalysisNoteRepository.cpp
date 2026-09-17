#include <storage/AnalysisNoteRepository.hpp>

#include <storage/Database.hpp>
#include <storage/StorageUtils.hpp>

#include <QDateTime>

namespace stutter {

namespace {
const QString kSelectColumns =
    QStringLiteral("id, binary_id, address, category, title, content, confidence, created_at, updated_at");

stutter::AnalysisNote readNote(Statement& statement) {
    stutter::AnalysisNote note;
    note.id = statement.text(0);
    note.binaryId = statement.text(1);
    note.address = static_cast<quint64>(statement.integer(2));
    note.category = statement.text(3);
    note.title = statement.text(4);
    note.content = statement.text(5);
    note.confidence = statement.number(6);
    note.createdAt = storage::fromStorage(statement.text(7));
    note.updatedAt = storage::fromStorage(statement.text(8));
    return note;
}
}

AnalysisNoteRepository::AnalysisNoteRepository(Database& database) : database_(database) {}

QVector<stutter::AnalysisNote> AnalysisNoteRepository::forBinary(const QString& binaryId) {
    QVector<stutter::AnalysisNote> notes;
    if (!database_.isOpen()) {
        return notes;
    }
    Statement statement(
        database_,
        QStringLiteral("SELECT %1 FROM analysis_notes WHERE binary_id = ?1 ORDER BY address ASC")
            .arg(kSelectColumns)
    );
    statement.bind(1, binaryId);
    while (statement.next()) {
        notes.append(readNote(statement));
    }
    return notes;
}

QVector<stutter::AnalysisNote> AnalysisNoteRepository::forBinaryAtAddress(
    const QString& binaryId,
    quint64 address
) {
    QVector<stutter::AnalysisNote> notes;
    if (!database_.isOpen()) {
        return notes;
    }
    Statement statement(
        database_,
        QStringLiteral(
            "SELECT %1 FROM analysis_notes WHERE binary_id = ?1 AND address = ?2 ORDER BY updated_at DESC"
        ).arg(kSelectColumns)
    );
    statement.bind(1, binaryId);
    statement.bind(2, static_cast<qint64>(address));
    while (statement.next()) {
        notes.append(readNote(statement));
    }
    return notes;
}

bool AnalysisNoteRepository::save(const stutter::AnalysisNote& note) {
    if (!database_.isOpen() || note.id.isEmpty()) {
        return false;
    }
    const QString createdAt = storage::toStorage(
        note.createdAt.isValid() ? note.createdAt : QDateTime::currentDateTimeUtc()
    );
    const QString updatedAt = storage::toStorage(
        note.updatedAt.isValid() ? note.updatedAt : QDateTime::currentDateTimeUtc()
    );

    Statement statement(
        database_,
        QStringLiteral(
            "INSERT INTO analysis_notes(id, binary_id, address, category, title, content, confidence, created_at, updated_at) "
            "VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9) "
            "ON CONFLICT(id) DO UPDATE SET "
            "address = excluded.address, category = excluded.category, title = excluded.title, "
            "content = excluded.content, confidence = excluded.confidence, updated_at = excluded.updated_at"
        )
    );
    statement.bind(1, note.id);
    statement.bind(2, note.binaryId);
    statement.bind(3, static_cast<qint64>(note.address));
    statement.bind(4, note.category);
    statement.bind(5, note.title);
    statement.bind(6, note.content);
    statement.bind(7, note.confidence);
    statement.bind(8, createdAt);
    statement.bind(9, updatedAt);
    statement.next();
    return statement.error().isEmpty();
}

bool AnalysisNoteRepository::remove(const QString& noteId) {
    if (!database_.isOpen()) {
        return false;
    }
    Statement statement(database_, QStringLiteral("DELETE FROM analysis_notes WHERE id = ?1"));
    statement.bind(1, noteId);
    statement.next();
    return statement.error().isEmpty();
}

}
