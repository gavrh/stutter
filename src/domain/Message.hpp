#pragma once

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>

namespace stutter {

enum class MessageRole {
    System,
    User,
    Assistant,
    Tool
};

struct Message {
    QString id;
    QString conversationId;
    MessageRole role = MessageRole::User;
    QString content;
    QString toolCallId;
    QString toolName;
    QJsonObject toolArguments;
    QJsonArray toolCalls;
    QDateTime createdAt;
};

}
