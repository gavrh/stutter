#include <chat/PromptBuilder.hpp>

#include <QFile>
#include <QJsonArray>
#include <QJsonObject>

static MessageRole providerRole(stutter::MessageRole role) {
    switch (role) {
    case stutter::MessageRole::System: return MessageRole::System;
    case stutter::MessageRole::User: return MessageRole::User;
    case stutter::MessageRole::Assistant: return MessageRole::Assistant;
    case stutter::MessageRole::Tool: return MessageRole::Tool;
    }
    return MessageRole::User;
}

PromptBuilder::PromptBuilder() {
    systemPrompt_ = loadResource(QStringLiteral(":/stutter/prompts/system.md"), error_);
    if (error_.isEmpty()) {
        reverseEngineerPrompt_ = loadResource(
            QStringLiteral(":/stutter/prompts/reverse_engineer.md"), error_
        );
    }
}

ChatRequest PromptBuilder::build(
    const ChatContext& context,
    const stutter::ProviderConfig& config
) const {
    ChatRequest request;
    request.model = config.modelId;
    request.effort = config.effort;
    request.maxTokens = config.maxOutputTokens;
    request.temperature = config.temperature;

    QString instructions = systemPrompt_ + QStringLiteral("\n\n") + reverseEngineerPrompt_;
    if (!context.analysisContext.isEmpty()) {
        instructions.append(QStringLiteral("\n\n## Current analysis context\n"));
        instructions.append(context.analysisContext);
    }
    request.messages.append({MessageRole::System, instructions});
    if (!context.summary.isEmpty()) {
        request.messages.append({
            MessageRole::System,
            QStringLiteral("Conversation summary:\n%1").arg(context.summary)
        });
    }
    for (const stutter::Message& message : context.messages) {
        ChatMessage providerMessage;
        providerMessage.role = providerRole(message.role);
        providerMessage.content = message.content;
        providerMessage.toolCallId = message.toolCallId;
        for (const QJsonValue& value : message.toolCalls) {
            const QJsonObject object = value.toObject();
            ToolCall call;
            call.id = object.value(QStringLiteral("id")).toString();
            call.name = object.value(QStringLiteral("name")).toString();
            call.arguments = object.value(QStringLiteral("arguments")).toObject();
            providerMessage.toolCalls.append(call);
        }
        request.messages.append(providerMessage);
    }
    return request;
}

QString PromptBuilder::loadResource(const QString& path, QString& error) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        error = QStringLiteral("Unable to load prompt resource: %1").arg(path);
        return {};
    }
    return QString::fromUtf8(file.readAll()).trimmed();
}
