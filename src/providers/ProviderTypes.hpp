#pragma once

#include <domain/Usage.hpp>
#include <tools/ToolCall.hpp>

#include <QJsonObject>
#include <QMetaType>
#include <QString>
#include <QUrl>
#include <QVector>

using stutter::ToolCall;

enum class ProviderType {
    OpenAI,
    Anthropic,
    Codex
};

enum class MessageRole {
    System,
    User,
    Assistant,
    Tool
};

struct ChatMessage {
    MessageRole role = MessageRole::User;
    QString content;
    QString toolCallId;
    QVector<ToolCall> toolCalls;
};

struct ToolDefinition {
    QString name;
    QString description;
    QJsonObject inputSchema;
};

struct ChatRequest {
    QString model;
    QVector<ChatMessage> messages;
    QVector<ToolDefinition> tools;
    QString effort;
    int maxTokens = 4096;
    double temperature = -1.0;
};

using TokenUsage = stutter::Usage;

struct ChatResponse {
    QString content;
    QVector<ToolCall> toolCalls;
    TokenUsage usage;
    QString stopReason;
};

struct ProviderError {
    QString code;
    QString message;
    int httpStatus = 0;
    bool retryable = false;
};

enum class ProviderEventType {
    TextDelta,
    ToolCallDelta,
    Completed,
    Error
};

struct ProviderEvent {
    ProviderEventType type = ProviderEventType::TextDelta;
    QString requestId;
    QString textDelta;
    int toolIndex = -1;
    QString toolCallId;
    QString toolName;
    QString argumentsDelta;
    TokenUsage usage;
    QString stopReason;
    ProviderError error;
};

Q_DECLARE_METATYPE(ChatResponse)
Q_DECLARE_METATYPE(ProviderError)
Q_DECLARE_METATYPE(ProviderEvent)
