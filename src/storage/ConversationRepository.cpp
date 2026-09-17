#include <storage/ConversationRepository.hpp>

#include <storage/Database.hpp>
#include <storage/StorageUtils.hpp>

#include <QDateTime>

namespace stutter {

namespace {
const QString kSelectColumns =
    QStringLiteral("id, binary_id, title, summary, created_at, updated_at");

stutter::Conversation readConversation(Statement& statement) {
    stutter::Conversation conversation;
    conversation.id = statement.text(0);
    conversation.binaryId = statement.text(1);
    conversation.title = statement.text(2);
    conversation.summary = statement.text(3);
    conversation.createdAt = storage::fromStorage(statement.text(4));
    conversation.updatedAt = storage::fromStorage(statement.text(5));
    return conversation;
}
}

ConversationRepository::ConversationRepository(Database& database) : database_(database) {}

QVector<stutter::Conversation> ConversationRepository::forBinary(const QString& binaryId) {
    QVector<stutter::Conversation> conversations;
    if (!database_.isOpen()) {
        return conversations;
    }
    Statement statement(
        database_,
        QStringLiteral("SELECT %1 FROM conversations WHERE binary_id = ?1 ORDER BY updated_at DESC")
            .arg(kSelectColumns)
    );
    statement.bind(1, binaryId);
    while (statement.next()) {
        conversations.append(readConversation(statement));
    }
    return conversations;
}

stutter::Conversation ConversationRepository::latestForBinary(const QString& binaryId) {
    stutter::Conversation conversation;
    if (!database_.isOpen()) {
        return conversation;
    }
    Statement statement(
        database_,
        QStringLiteral(
            "SELECT %1 FROM conversations WHERE binary_id = ?1 ORDER BY updated_at DESC LIMIT 1"
        ).arg(kSelectColumns)
    );
    statement.bind(1, binaryId);
    if (statement.next()) {
        conversation = readConversation(statement);
    }
    return conversation;
}

bool ConversationRepository::save(const stutter::Conversation& conversation) {
    if (!database_.isOpen() || conversation.id.isEmpty()) {
        return false;
    }
    const QString createdAt = storage::toStorage(
        conversation.createdAt.isValid() ? conversation.createdAt : QDateTime::currentDateTimeUtc()
    );
    const QString updatedAt = storage::toStorage(
        conversation.updatedAt.isValid() ? conversation.updatedAt : QDateTime::currentDateTimeUtc()
    );

    Statement statement(
        database_,
        QStringLiteral(
            "INSERT INTO conversations(id, binary_id, title, summary, created_at, updated_at) "
            "VALUES (?1, ?2, ?3, ?4, ?5, ?6) "
            "ON CONFLICT(id) DO UPDATE SET "
            "binary_id = excluded.binary_id, title = excluded.title, summary = excluded.summary, "
            "updated_at = excluded.updated_at"
        )
    );
    statement.bind(1, conversation.id);
    statement.bind(2, conversation.binaryId);
    statement.bind(3, conversation.title);
    statement.bind(4, conversation.summary);
    statement.bind(5, createdAt);
    statement.bind(6, updatedAt);
    statement.next();
    return statement.error().isEmpty();
}

bool ConversationRepository::remove(const QString& conversationId) {
    if (!database_.isOpen()) {
        return false;
    }
    Statement statement(database_, QStringLiteral("DELETE FROM conversations WHERE id = ?1"));
    statement.bind(1, conversationId);
    statement.next();
    return statement.error().isEmpty();
}

bool ConversationRepository::updateSummary(const QString& conversationId, const QString& summary) {
    if (!database_.isOpen()) {
        return false;
    }
    Statement statement(
        database_,
        QStringLiteral(
            "UPDATE conversations SET summary = ?1, updated_at = ?2 WHERE id = ?3"
        )
    );
    statement.bind(1, summary);
    statement.bind(2, storage::toStorage(QDateTime::currentDateTimeUtc()));
    statement.bind(3, conversationId);
    statement.next();
    return statement.error().isEmpty();
}

}
