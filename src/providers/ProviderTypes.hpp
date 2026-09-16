#pragma once

#include <QJsonObject>
#include <QMetaType>
#include <QString>
#include <QUrl>
#include <QVector>

enum class ProviderType {
    OpenAI,
    Anthropic
};

enum class MessageRole {
    System,
    User,
    Assistant,
    Tool
};

struct ToolCall {
    QString id;
    QString name;
    QJsonObject arguments;
    QString rawArguments;
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
    int maxTokens = 4096;
    double temperature = -1.0;
};

struct TokenUsage {
    qint64 inputTokens = 0;
    qint64 outputTokens = 0;
};

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

struct ProviderConfig {
    ProviderType type = ProviderType::OpenAI;
    QString apiKey;
    QUrl baseUrl;
    QString anthropicVersion = QStringLiteral("2023-06-01");
};

Q_DECLARE_METATYPE(ChatResponse)
Q_DECLARE_METATYPE(ProviderError)
Q_DECLARE_METATYPE(ProviderEvent)
