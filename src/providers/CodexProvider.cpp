#include <providers/CodexProvider.hpp>

#include <QTimer>
#include <QUuid>

namespace {
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
        response_.content.append(delta);
        ProviderEvent event;
        event.type = ProviderEventType::TextDelta;
        event.requestId = activeRequestId_;
        event.textDelta = delta;
        emit eventReceived(event);
    });
    connect(&session_, &CodexSession::tokenUsage, this, [this](qint64 inputTokens, qint64 outputTokens) {
        response_.usage.inputTokens = inputTokens;
        response_.usage.outputTokens = outputTokens;
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
    pendingEffort_ = request.effort;
    response_ = {};
    session_.startThread(request.model, {});
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
        prompt.append(QStringLiteral("## %1\n%2\n\n").arg(roleName(message.role), message.content));
    }
    return prompt.trimmed();
}

void CodexProvider::failActive(const QString& message) {
    if (activeRequestId_.isEmpty()) return;
    const QString requestId = activeRequestId_;
    activeRequestId_.clear();
    pendingPrompt_.clear();
    pendingEffort_.clear();
    ProviderError error;
    error.code = QStringLiteral("codex_error");
    error.message = message;
    emit failed(requestId, error);
}
