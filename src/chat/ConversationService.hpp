#pragma once

#include <domain/BinaryIdentity.hpp>
#include <domain/Conversation.hpp>
#include <domain/Message.hpp>

#include <QObject>
#include <QVector>

namespace stutter {
class BinaryRepository;
class ConversationRepository;
class MessageRepository;
}

class ConversationService final : public QObject {
    Q_OBJECT

public:
    explicit ConversationService(QObject* parent = nullptr);

    void setRepositories(
        stutter::BinaryRepository* binaries,
        stutter::ConversationRepository* conversations,
        stutter::MessageRepository* messages
    );
    void setBinary(const stutter::BinaryIdentity& identity);

    const stutter::BinaryIdentity& binary() const { return binary_; }
    bool hasBinary() const { return !binary_.id.isEmpty(); }

    const stutter::Conversation& currentConversation() const { return conversation_; }
    const QVector<stutter::Message>& messages() const { return messages_; }
    void ensureConversation(const QString& firstMessage = {});
    void startConversation(const QString& title = {});
    stutter::Message appendMessage(stutter::MessageRole role, const QString& content);
    void appendMessage(const stutter::Message& message);
    void setSummary(const QString& summary);
    void clear();

signals:
    void conversationChanged(const stutter::Conversation& conversation);
    void messageAdded(const stutter::Message& message);
    void conversationLoaded();
    void conversationCleared();

private:
    stutter::BinaryRepository* binaries_ = nullptr;
    stutter::ConversationRepository* conversationRepository_ = nullptr;
    stutter::MessageRepository* messageRepository_ = nullptr;
    stutter::BinaryIdentity binary_;
    stutter::Conversation conversation_;
    QVector<stutter::Message> messages_;
};
