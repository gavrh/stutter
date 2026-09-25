#include <chat/ContextBuilder.hpp>

#include <QtGlobal>

ChatContext ContextBuilder::build(
    const QVector<stutter::Message>& messages,
    const QString& summary,
    const QString& analysisContext,
    const stutter::Model& model
) const {
    ChatContext context;
    context.summary = summary;
    context.analysisContext = analysisContext;

    const qint64 contextWindow = model.contextWindow > 0 ? model.contextWindow : 128000;
    const qint64 requestedOutput = model.maxOutputTokens > 0
        ? qMin<qint64>(model.maxOutputTokens, stutter::maxRequestedOutputTokens)
        : 4096;
    const qint64 outputReserve = qMin<qint64>(requestedOutput, contextWindow / 4);
    qint64 remaining = qMax<qint64>(4096, contextWindow - outputReserve - 2048);
    remaining -= estimateTokens(summary) + estimateTokens(analysisContext);

    for (auto iterator = messages.crbegin(); iterator != messages.crend(); ++iterator) {
        const qint64 tokens = estimateTokens(iterator->content) + 8;
        if (tokens > remaining && !context.messages.isEmpty()) {
            context.historyTruncated = true;
            break;
        }
        context.messages.prepend(*iterator);
        remaining -= tokens;
    }
    // A truncated window must not start with a tool result whose assistant
    // tool call was dropped.
    while (!context.messages.isEmpty()
           && context.messages.first().role == stutter::MessageRole::Tool) {
        context.messages.removeFirst();
        context.historyTruncated = true;
    }
    context.historyTruncated = context.historyTruncated || context.messages.size() < messages.size();
    return context;
}

qint64 ContextBuilder::estimateTokens(const QString& text) {
    return qMax<qint64>(1, (text.size() + 3) / 4);
}
