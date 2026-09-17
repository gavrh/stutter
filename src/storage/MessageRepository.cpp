#include <storage/MessageRepository.hpp>

#include <storage/Database.hpp>
#include <storage/StorageUtils.hpp>

#include <QDateTime>
#include <QJsonDocument>

namespace stutter {

namespace {
const QString kSelectColumns =
    QStringLiteral("id, conversation_id, role, content, tool_call_id, tool_name, tool_arguments, created_at");

stutter::Message readMessage(Statement& statement) {
    stutter::Message message;
    message.id = statement.text(0);
    message.conversationId = statement.text(1);
    message.role = storage::roleFromString(statement.text(2));
    message.content = statement.text(3);
    message.toolCallId = statement.text(4);
    message.toolName = statement.text(5);
    if (!statement.isNull(6)) {
        message.toolArguments = QJsonDocument::fromJson(statement.text(6).toUtf8()).object();
    }
    message.createdAt = storage::fromStorage(statement.text(7));
    return message;
}

QString serializeArguments(const QJsonObject& arguments) {
    if (arguments.isEmpty()) {
        return {};
    }
    return QString::fromUtf8(QJsonDocument(arguments).toJson(QJsonDocument::Compact));
}
}

MessageRepository::MessageRepository(Database& database) : database_(database) {}

QVector<stutter::Message> MessageRepository::forConversation(const QString& conversationId) {
    QVector<stutter::Message> messages;
    if (!database_.isOpen()) {
        return messages;
    }
    Statement statement(
        database_,
        QStringLiteral("SELECT %1 FROM messages WHERE conversation_id = ?1 ORDER BY created_at ASC")
            .arg(kSelectColumns)
    );
    statement.bind(1, conversationId);
    while (statement.next()) {
        messages.append(readMessage(statement));
    }
    return messages;
}

bool MessageRepository::save(const stutter::Message& message) {
    if (!database_.isOpen() || message.id.isEmpty()) {
        return false;
    }
    const QString createdAt = storage::toStorage(
        message.createdAt.isValid() ? message.createdAt : QDateTime::currentDateTimeUtc()
    );
    const QString arguments = serializeArguments(message.toolArguments);

    Statement statement(
        database_,
        QStringLiteral(
            "INSERT INTO messages(id, conversation_id, role, content, tool_call_id, tool_name, tool_arguments, created_at) "
            "VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8) "
            "ON CONFLICT(id) DO UPDATE SET "
            "content = excluded.content, tool_call_id = excluded.tool_call_id, "
            "tool_name = excluded.tool_name, tool_arguments = excluded.tool_arguments"
        )
    );
    statement.bind(1, message.id);
    statement.bind(2, message.conversationId);
    statement.bind(3, storage::roleToString(message.role));
    statement.bind(4, message.content);
    if (message.toolCallId.isEmpty()) statement.bindNull(5); else statement.bind(5, message.toolCallId);
    if (message.toolName.isEmpty()) statement.bindNull(6); else statement.bind(6, message.toolName);
    if (arguments.isEmpty()) statement.bindNull(7); else statement.bind(7, arguments);
    statement.bind(8, createdAt);
    statement.next();
    return statement.error().isEmpty();
}

bool MessageRepository::removeForConversation(const QString& conversationId) {
    if (!database_.isOpen()) {
        return false;
    }
    Statement statement(database_, QStringLiteral("DELETE FROM messages WHERE conversation_id = ?1"));
    statement.bind(1, conversationId);
    statement.next();
    return statement.error().isEmpty();
}

}
