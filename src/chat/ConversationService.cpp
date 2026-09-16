#include <chat/ConversationService.hpp>

#include <QDateTime>
#include <QUuid>

ConversationService::ConversationService(QObject* parent) : QObject(parent) {}

void ConversationService::ensureConversation(const QString& firstMessage) {
    if (conversation_.isValid()) return;
    QString title = firstMessage.simplified();
    if (title.size() > 60) title = title.left(57) + QStringLiteral("...");
    startConversation(title);
}

void ConversationService::startConversation(const QString& title) {
    conversation_ = {};
    conversation_.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    conversation_.title = title.isEmpty() ? tr("New conversation") : title;
    conversation_.createdAt = QDateTime::currentDateTimeUtc();
    conversation_.updatedAt = conversation_.createdAt;
    messages_.clear();
    emit conversationChanged(conversation_);
}

stutter::Message ConversationService::appendMessage(
    stutter::MessageRole role,
    const QString& content
) {
    ensureConversation(content);
    stutter::Message message;
    message.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    message.conversationId = conversation_.id;
    message.role = role;
    message.content = content;
    message.createdAt = QDateTime::currentDateTimeUtc();
    appendMessage(message);
    return message;
}

void ConversationService::appendMessage(const stutter::Message& message) {
    ensureConversation(message.content);
    messages_.append(message);
    conversation_.updatedAt = QDateTime::currentDateTimeUtc();
    emit messageAdded(message);
}

void ConversationService::setSummary(const QString& summary) {
    ensureConversation();
    conversation_.summary = summary;
    conversation_.updatedAt = QDateTime::currentDateTimeUtc();
    emit conversationChanged(conversation_);
}

void ConversationService::clear() {
    conversation_ = {};
    messages_.clear();
    emit conversationCleared();
}
