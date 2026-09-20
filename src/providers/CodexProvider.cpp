#include <providers/CodexProvider.hpp>

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QStringList>
#include <QTimer>
#include <QUuid>

namespace {
bool isActivityItem(const QJsonObject& item) {
    const QString type = item.value(QStringLiteral("type")).toString();
    return !type.isEmpty()
        && type != QStringLiteral("agentMessage")
        && type != QStringLiteral("reasoning")
        && type != QStringLiteral("userMessage");
}

void appendItemField(const QJsonObject& item, const char* key, QStringList& parts) {
    const QJsonValue value = item.value(QLatin1String(key));
    if (value.isString()) {
        const QString text = value.toString();
        if (!text.isEmpty()) parts.append(text);
    } else if (value.isObject()) {
        parts.append(QString::fromUtf8(QJsonDocument(value.toObject()).toJson(QJsonDocument::Compact)));
    } else if (value.isArray()) {
        parts.append(QString::fromUtf8(QJsonDocument(value.toArray()).toJson(QJsonDocument::Compact)));
    }
}

QString firstString(const QJsonObject& object, const char* key) {
    const QJsonValue value = object.value(QLatin1String(key));
    return value.isString() ? value.toString() : QString();
}

QString itemSummary(const QJsonObject& item) {
    const QString type = item.value(QStringLiteral("type")).toString();
    QStringList parts;

    if (type == QStringLiteral("commandExecution")) {
        appendItemField(item, "command", parts);
        appendItemField(item, "cwd", parts);
    } else if (type == QStringLiteral("fileChange")) {
        appendItemField(item, "path", parts);
    } else if (type == QStringLiteral("webSearch")) {
        const QJsonObject action = item.value(QStringLiteral("action")).toObject();
        QString url = firstString(action, "url");
        if (url.isEmpty()) url = firstString(item, "url");
        if (!url.isEmpty()) parts.append(url);
    } else if (type == QStringLiteral("mcpToolCall")) {
        appendItemField(item, "server", parts);
        appendItemField(item, "tool", parts);
        appendItemField(item, "arguments", parts);
    }

    if (parts.isEmpty()) {
        const QJsonObject arguments = item.value(QStringLiteral("arguments")).toObject();
        if (!arguments.isEmpty()) {
            parts.append(QString::fromUtf8(
                QJsonDocument(arguments).toJson(QJsonDocument::Compact)
            ));
        }
    }
    return parts.join(QLatin1Char('\n'));
}

QString stripCitations(const QString& text) {
    static const QRegularExpression pattern(
        QStringLiteral(R"((?:cite\s*)?turn\d+[a-z]+\d+)"),
        QRegularExpression::CaseInsensitiveOption
    );
    QString result = text;
    result.remove(pattern);
    return result;
}

QString itemResult(const QJsonObject& item) {
    const char* keys[] = {"aggregatedOutput", "output", "result", "text"};
    for (const char* key : keys) {
        const QString value = item.value(QLatin1String(key)).toString();
        if (!value.isEmpty()) return value;
    }
    return {};
}

QString roleName(MessageRole role) {
    switch (role) {
    case MessageRole::System: return QStringLiteral("System");
    case MessageRole::User: return QStringLiteral("User");
    case MessageRole::Assistant: return QStringLiteral("Assistant");
    case MessageRole::Tool: return QStringLiteral("Tool");
    }
    return QStringLiteral("User");
}
}

CodexProvider::CodexProvider(QObject* parent)
    : Provider(parent),
      process_(this),
      rpc_(process_, this),
      auth_(process_, rpc_, this),
      session_(rpc_, this) {
    connect(&auth_, &CodexAuthService::statusChanged,
            this, &CodexProvider::connectionStatusChanged);
    connect(&session_, &CodexSession::threadStarted, this, [this] {
        session_.startTurn(pendingPrompt_, pendingEffort_);
    });
    connect(&session_, &CodexSession::textDelta, this, [this](const QString& delta) {
        if (activeRequestId_.isEmpty()) return;
        const QString clean = stripCitations(delta);
        response_.content.append(clean);
        if (clean.isEmpty()) return;
        ProviderEvent event;
        event.type = ProviderEventType::TextDelta;
        event.requestId = activeRequestId_;
        event.textDelta = clean;
        emit eventReceived(event);
    });
    connect(&session_, &CodexSession::tokenUsage, this, [this](qint64 inputTokens, qint64 outputTokens) {
        response_.usage.inputTokens = inputTokens;
        response_.usage.outputTokens = outputTokens;
    });
    connect(&session_, &CodexSession::itemStarted, this,
        [this](const QString& itemId, const QJsonObject& item) {
            if (!isActivityItem(item)) return;
            const QString detail = itemSummary(item);
            if (detail.isEmpty()) return;
            stutter::ToolActivity activity;
            activity.id = itemId;
            activity.name = item.value(QStringLiteral("type")).toString();
            activity.status = stutter::ToolActivityStatus::Running;
            activity.detail = detail;
            activity.startedAt = QDateTime::currentDateTimeUtc();
            emit activityStarted(activity);
        });
    connect(&session_, &CodexSession::itemCompleted, this,
        [this](const QString& itemId, const QJsonObject& item) {
            if (!isActivityItem(item)) return;
            const QString detail = itemSummary(item);
            if (detail.isEmpty()) return;
            stutter::ToolActivity activity;
            activity.id = itemId;
            activity.name = item.value(QStringLiteral("type")).toString();
            activity.status = item.value(QStringLiteral("error")).toObject().isEmpty()
                ? stutter::ToolActivityStatus::Succeeded
                : stutter::ToolActivityStatus::Failed;
            activity.detail = detail;
            activity.result = itemResult(item);
            activity.finishedAt = QDateTime::currentDateTimeUtc();
            emit activityFinished(activity);
        });
    connect(&session_, &CodexSession::turnCompleted, this, [this](const QString& status) {
        if (activeRequestId_.isEmpty()) return;
        if (status != QStringLiteral("completed")) {
            failActive(tr("Codex turn ended with status: %1").arg(status));
            return;
        }
        response_.stopReason = status;
        ProviderEvent event;
        event.type = ProviderEventType::Completed;
        event.requestId = activeRequestId_;
        event.stopReason = status;
        emit eventReceived(event);
        const QString requestId = activeRequestId_;
        activeRequestId_.clear();
        pendingPrompt_.clear();
        pendingInstructions_.clear();
        pendingEffort_.clear();
        emit finished(requestId, response_);
    });
    connect(&session_, &CodexSession::errorOccurred, this, &CodexProvider::failActive);
}

QString CodexProvider::send(const ChatRequest& request) {
    const QString requestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    if (!isAvailable() || !isConnected() || !activeRequestId_.isEmpty() || request.model.isEmpty()) {
        QString message;
        if (!isAvailable()) message = tr("Codex CLI is not installed or is not on PATH");
        else if (!isConnected()) message = tr("Connect a ChatGPT account in Settings first");
        else if (!activeRequestId_.isEmpty()) message = tr("A Codex request is already running");
        else message = tr("Model is empty");
        QTimer::singleShot(0, this, [this, requestId, message] {
            ProviderError error;
            error.code = QStringLiteral("codex_unavailable");
            error.message = message;
            emit failed(requestId, error);
        });
        return requestId;
    }

    activeRequestId_ = requestId;
    pendingPrompt_ = promptFor(request);
    pendingInstructions_ = instructionsFor(request);
    pendingEffort_ = request.effort;
    response_ = {};
    session_.startThread(request.model, {}, pendingInstructions_);
    return requestId;
}

void CodexProvider::cancel(const QString& requestId) {
    if (requestId == activeRequestId_) session_.interrupt();
}

void CodexProvider::disconnect() {
    const bool hadActiveRequest = !activeRequestId_.isEmpty();
    auth_.disconnect();
    if (hadActiveRequest) failActive(tr("Codex disconnected because the provider changed"));
}

QString CodexProvider::promptFor(const ChatRequest& request) const {
    QString prompt;
    for (const ChatMessage& message : request.messages) {
        if (message.role == MessageRole::System) continue;
        prompt.append(QStringLiteral("## %1\n%2\n\n").arg(roleName(message.role), message.content));
    }
    return prompt.trimmed();
}

QString CodexProvider::instructionsFor(const ChatRequest& request) const {
    QString instructions;
    for (const ChatMessage& message : request.messages) {
        if (message.role != MessageRole::System) continue;
        if (message.content.isEmpty()) continue;
        if (!instructions.isEmpty()) instructions.append(QStringLiteral("\n\n"));
        instructions.append(message.content);
    }
    return instructions;
}

void CodexProvider::failActive(const QString& message) {
    if (activeRequestId_.isEmpty()) return;
    const QString requestId = activeRequestId_;
    activeRequestId_.clear();
    pendingPrompt_.clear();
    pendingInstructions_.clear();
    pendingEffort_.clear();
    ProviderError error;
    error.code = QStringLiteral("codex_error");
    error.message = message;
    emit failed(requestId, error);
}
