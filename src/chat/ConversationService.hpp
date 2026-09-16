#pragma once

#include <domain/Conversation.hpp>
#include <domain/Message.hpp>

#include <QObject>
#include <QVector>

class ConversationService final : public QObject {
    Q_OBJECT

public:
    explicit ConversationService(QObject* parent = nullptr);

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
    void conversationCleared();

private:
    stutter::Conversation conversation_;
    QVector<stutter::Message> messages_;
};
