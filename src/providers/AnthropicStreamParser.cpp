#include <providers/StreamParser.hpp>

#include <QJsonDocument>

QVector<ProviderEvent> AnthropicStreamParser::push(const QByteArray& bytes) {
    buffer_.append(bytes);
    return consume(false);
}

QVector<ProviderEvent> AnthropicStreamParser::finish() {
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

QVector<ProviderEvent> AnthropicStreamParser::consume(bool flush) {
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
            events += parseData(line.mid(5).trimmed());
        }
    }
    return events;
}

QVector<ProviderEvent> AnthropicStreamParser::parseData(const QByteArray& data) {
    QVector<ProviderEvent> events;
    QJsonParseError parseError;
    const QJsonObject root = QJsonDocument::fromJson(data, &parseError).object();
    if (parseError.error != QJsonParseError::NoError) {
        return events;
    }

    const QString type = root.value(QStringLiteral("type")).toString();
    if (type == QStringLiteral("error")) {
        const QJsonObject errorObject = root.value(QStringLiteral("error")).toObject();
        ProviderEvent event;
        event.type = ProviderEventType::Error;
        event.error.code = errorObject.value(QStringLiteral("type")).toString();
        event.error.message = errorObject.value(QStringLiteral("message")).toString();
        events.append(event);
    } else if (type == QStringLiteral("message_start")) {
        const QJsonObject usage = root.value(QStringLiteral("message")).toObject()
            .value(QStringLiteral("usage")).toObject();
        usage_.inputTokens = static_cast<qint64>(usage.value(QStringLiteral("input_tokens")).toDouble());
    } else if (type == QStringLiteral("content_block_start")) {
        const int index = root.value(QStringLiteral("index")).toInt();
        const QJsonObject block = root.value(QStringLiteral("content_block")).toObject();
        const QString blockType = block.value(QStringLiteral("type")).toString();
        if (blockType == QStringLiteral("text")) {
            const QString text = block.value(QStringLiteral("text")).toString();
            if (!text.isEmpty()) {
                ProviderEvent event;
                event.type = ProviderEventType::TextDelta;
                event.textDelta = text;
                events.append(event);
            }
        } else if (blockType == QStringLiteral("tool_use")) {
            toolIds_[index] = block.value(QStringLiteral("id")).toString();
            toolNames_[index] = block.value(QStringLiteral("name")).toString();
            ProviderEvent event;
            event.type = ProviderEventType::ToolCallDelta;
            event.toolIndex = index;
            event.toolCallId = toolIds_.value(index);
            event.toolName = toolNames_.value(index);
            events.append(event);
        }
    } else if (type == QStringLiteral("content_block_delta")) {
        const int index = root.value(QStringLiteral("index")).toInt();
        const QJsonObject delta = root.value(QStringLiteral("delta")).toObject();
        const QString deltaType = delta.value(QStringLiteral("type")).toString();
        ProviderEvent event;
        if (deltaType == QStringLiteral("text_delta")) {
            event.type = ProviderEventType::TextDelta;
            event.textDelta = delta.value(QStringLiteral("text")).toString();
        } else if (deltaType == QStringLiteral("input_json_delta")) {
            event.type = ProviderEventType::ToolCallDelta;
            event.toolIndex = index;
            event.toolCallId = toolIds_.value(index);
            event.toolName = toolNames_.value(index);
            event.argumentsDelta = delta.value(QStringLiteral("partial_json")).toString();
        } else {
            return events;
        }
        events.append(event);
    } else if (type == QStringLiteral("message_delta")) {
        stopReason_ = root.value(QStringLiteral("delta")).toObject()
            .value(QStringLiteral("stop_reason")).toString();
        const QJsonObject usage = root.value(QStringLiteral("usage")).toObject();
        usage_.outputTokens = static_cast<qint64>(usage.value(QStringLiteral("output_tokens")).toDouble());
    } else if (type == QStringLiteral("message_stop") && !completed_) {
        completed_ = true;
        ProviderEvent event;
        event.type = ProviderEventType::Completed;
        event.usage = usage_;
        event.stopReason = stopReason_;
        events.append(event);
    }
    return events;
}
