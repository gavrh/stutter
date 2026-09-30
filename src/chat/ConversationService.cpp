#include <chat/ConversationService.hpp>

#include <storage/BinaryRepository.hpp>
#include <storage/ConversationRepository.hpp>
#include <storage/MessageRepository.hpp>

#include <QDateTime>
#include <QUuid>

ConversationService::ConversationService(QObject* parent) : QObject(parent) {}

void ConversationService::setRepositories(
    stutter::BinaryRepository* binaries,
    stutter::ConversationRepository* conversations,
    stutter::MessageRepository* messages
) {
    binaries_ = binaries;
    conversationRepository_ = conversations;
    messageRepository_ = messages;
}

void ConversationService::setBinary(const stutter::BinaryIdentity& identity) {
    if (!identity.isResolvable()) {
        return;
    }
    const bool samePath = !identity.path.isEmpty() && binary_.path == identity.path;
    const bool sameHash = !identity.sha256.isEmpty() && binary_.sha256 == identity.sha256;
    if (binary_.isResolvable() && (samePath || sameHash)) {
        return;
    }

    stutter::BinaryIdentity resolved = identity;
    if (binaries_) {
        resolved = binaries_->resolve(identity);
    }
    binary_ = resolved;

    conversation_ = {};
    messages_.clear();
    if (conversationRepository_ && messageRepository_ && !binary_.id.isEmpty()) {
        conversation_ = conversationRepository_->latestForBinary(binary_.id);
        if (conversation_.isValid()) {
            messages_ = messageRepository_->forConversation(conversation_.id);
        }
    }
    emit conversationChanged(conversation_);
    emit conversationLoaded();
}

void ConversationService::ensureConversation(const QString& firstMessage) {
    if (conversation_.isValid()) return;
    QString title = firstMessage.simplified();
    if (title.size() > 60) title = title.left(57) + QStringLiteral("...");
    startConversation(title);
}

void ConversationService::startConversation(const QString& title) {
    conversation_ = {};
    conversation_.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    conversation_.binaryId = binary_.id;
    conversation_.title = title.isEmpty() ? tr("New conversation") : title;
    conversation_.createdAt = QDateTime::currentDateTimeUtc();
    conversation_.updatedAt = conversation_.createdAt;
    messages_.clear();
    if (conversationRepository_) {
        conversationRepository_->save(conversation_);
    }
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

    stutter::Message entry = message;
    if (entry.id.isEmpty()) {
        entry.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    if (entry.conversationId.isEmpty()) {
        entry.conversationId = conversation_.id;
    }
    if (!entry.createdAt.isValid()) {
        entry.createdAt = QDateTime::currentDateTimeUtc();
    }

    messages_.append(entry);
    conversation_.updatedAt = QDateTime::currentDateTimeUtc();
    if (messageRepository_) {
        messageRepository_->save(entry);
    }
    if (conversationRepository_) {
        conversationRepository_->save(conversation_);
    }
    emit messageAdded(entry);
}

void ConversationService::setSummary(const QString& summary) {
    ensureConversation();
    conversation_.summary = summary;
    conversation_.updatedAt = QDateTime::currentDateTimeUtc();
    if (conversationRepository_) {
        conversationRepository_->updateSummary(conversation_.id, summary);
    }
    emit conversationChanged(conversation_);
}

void ConversationService::clear() {
    if (conversation_.isValid()) {
        if (messageRepository_) {
            messageRepository_->removeForConversation(conversation_.id);
        }
        if (conversationRepository_) {
            conversationRepository_->remove(conversation_.id);
        }
    }
    conversation_ = {};
    messages_.clear();
    emit conversationCleared();
}
