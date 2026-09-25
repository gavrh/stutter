#include <chat/HistorySummarizer.hpp>

#include <chat/ContextBuilder.hpp>

#include <QFile>

static QString roleName(stutter::MessageRole role) {
    switch (role) {
    case stutter::MessageRole::System: return QStringLiteral("System");
    case stutter::MessageRole::User: return QStringLiteral("User");
    case stutter::MessageRole::Assistant: return QStringLiteral("Assistant");
    case stutter::MessageRole::Tool: return QStringLiteral("Tool");
    }
    return {};
}

HistorySummarizer::HistorySummarizer() {
    QFile file(QStringLiteral(":/stutter/prompts/summarize_history.md"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        error_ = QStringLiteral("Unable to load history summarization prompt");
        return;
    }
    prompt_ = QString::fromUtf8(file.readAll()).trimmed();
}

bool HistorySummarizer::shouldSummarize(
    const QVector<stutter::Message>& messages,
    qint64 tokenBudget
) const {
    qint64 tokens = 0;
    for (const stutter::Message& message : messages) {
        tokens += ContextBuilder::estimateTokens(message.content) + 8;
    }
    return tokens > tokenBudget;
}

QVector<stutter::Message> HistorySummarizer::messagesForSummary(
    const QVector<stutter::Message>& messages,
    qint64 targetTokens
) const {
    QVector<stutter::Message> selected;
    qint64 tokens = 0;
    for (const stutter::Message& message : messages) {
        const qint64 messageTokens = ContextBuilder::estimateTokens(message.content) + 8;
        if (tokens + messageTokens > targetTokens && !selected.isEmpty()) break;
        selected.append(message);
        tokens += messageTokens;
    }
    return selected;
}

ChatRequest HistorySummarizer::buildRequest(
    const QVector<stutter::Message>& messages,
    const QString& model
) const {
    QString history;
    for (const stutter::Message& message : messages) {
        history.append(QStringLiteral("## %1\n%2\n\n").arg(
            roleName(message.role),
            message.content
        ));
    }
    ChatRequest request;
    request.model = model;
    request.messages.append({MessageRole::System, prompt_});
    request.messages.append({MessageRole::User, history.trimmed()});
    return request;
}
