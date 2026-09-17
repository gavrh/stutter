#pragma once

#include <domain/Message.hpp>

#include <QDateTime>
#include <QString>

namespace stutter::storage {

inline QString roleToString(MessageRole role) {
    switch (role) {
    case MessageRole::System: return QStringLiteral("system");
    case MessageRole::User: return QStringLiteral("user");
    case MessageRole::Assistant: return QStringLiteral("assistant");
    case MessageRole::Tool: return QStringLiteral("tool");
    }
    return QStringLiteral("user");
}

inline MessageRole roleFromString(const QString& role) {
    if (role == QStringLiteral("system")) return MessageRole::System;
    if (role == QStringLiteral("assistant")) return MessageRole::Assistant;
    if (role == QStringLiteral("tool")) return MessageRole::Tool;
    return MessageRole::User;
}

inline QString toStorage(const QDateTime& dateTime) {
    return dateTime.toUTC().toString(Qt::ISODateWithMs);
}

inline QDateTime fromStorage(const QString& value) {
    return QDateTime::fromString(value, Qt::ISODateWithMs);
}

}
