#include <providers/StreamParser.hpp>

#include <QJsonArray>
#include <QJsonDocument>

QVector<ProviderEvent> OpenAIStreamParser::push(const QByteArray& bytes) {
    buffer_.append(bytes);
    return consume(false);
}

QVector<ProviderEvent> OpenAIStreamParser::finish() {
    QVector<ProviderEvent> events = consume(true);
    if (!completed_) {
        completed_ = true;
        ProviderEvent event;
        event.type = ProviderEventType::Completed;
        event.usage = usage_;
        event.stopReason = stopReason_;
        events.append(event);
    }
    return events;
}

QVector<ProviderEvent> OpenAIStreamParser::consume(bool flush) {
    QVector<ProviderEvent> events;
    while (true) {
        const auto newline = buffer_.indexOf('\n');
        if (newline < 0) {
            if (!flush || buffer_.isEmpty()) {
                break;
            }
        }

        QByteArray line;
        if (newline < 0) {
            line = buffer_;
            buffer_.clear();
        } else {
            line = buffer_.left(newline);
            buffer_.remove(0, newline + 1);
        }
        if (line.endsWith('\r')) {
            line.chop(1);
        }
        if (line.startsWith("data:")) {
            const auto parsed = parseData(line.mid(5).trimmed());
            events += parsed;
        }
    }
    return events;
}

QVector<ProviderEvent> OpenAIStreamParser::parseData(const QByteArray& data) {
    QVector<ProviderEvent> events;
    if (data == "[DONE]") {
        if (!completed_) {
            completed_ = true;
            ProviderEvent event;
            event.type = ProviderEventType::Completed;
            event.usage = usage_;
            event.stopReason = stopReason_;
            events.append(event);
        }
        return events;
    }

    QJsonParseError parseError;
    const QJsonObject root = QJsonDocument::fromJson(data, &parseError).object();
    if (parseError.error != QJsonParseError::NoError) {
        return events;
    }

    if (root.contains(QStringLiteral("error"))) {
        const QJsonObject errorObject = root.value(QStringLiteral("error")).toObject();
        ProviderEvent event;
        event.type = ProviderEventType::Error;
        event.error.code = errorObject.value(QStringLiteral("code")).toVariant().toString();
        event.error.message = errorObject.value(QStringLiteral("message")).toString();
        events.append(event);
        return events;
    }

    const QJsonObject usage = root.value(QStringLiteral("usage")).toObject();
    if (!usage.isEmpty()) {
        usage_.inputTokens = static_cast<qint64>(usage.value(QStringLiteral("prompt_tokens")).toDouble());
        usage_.outputTokens = static_cast<qint64>(usage.value(QStringLiteral("completion_tokens")).toDouble());
    }

    const QJsonArray choices = root.value(QStringLiteral("choices")).toArray();
    if (choices.isEmpty()) {
        return events;
    }

    const QJsonObject choice = choices.first().toObject();
    if (!choice.value(QStringLiteral("finish_reason")).isNull()) {
        stopReason_ = choice.value(QStringLiteral("finish_reason")).toString();
    }
    const QJsonObject delta = choice.value(QStringLiteral("delta")).toObject();
    const QString content = delta.value(QStringLiteral("content")).toString();
    if (!content.isEmpty()) {
        ProviderEvent event;
        event.type = ProviderEventType::TextDelta;
        event.textDelta = content;
        events.append(event);
    }

    const QJsonArray toolCalls = delta.value(QStringLiteral("tool_calls")).toArray();
    for (const QJsonValue& value : toolCalls) {
        const QJsonObject tool = value.toObject();
        const QJsonObject function = tool.value(QStringLiteral("function")).toObject();
        ProviderEvent event;
        event.type = ProviderEventType::ToolCallDelta;
        event.toolIndex = tool.value(QStringLiteral("index")).toInt();
        event.toolCallId = tool.value(QStringLiteral("id")).toString();
        event.toolName = function.value(QStringLiteral("name")).toString();
        event.argumentsDelta = function.value(QStringLiteral("arguments")).toString();
        events.append(event);
    }
    return events;
}
