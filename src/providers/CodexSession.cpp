#include <providers/CodexSession.hpp>

#include <providers/CodexRpcClient.hpp>

#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>

namespace {
void readTokenUsage(const QJsonObject& object, qint64& inputTokens, qint64& outputTokens) {
    inputTokens = static_cast<qint64>(object.value(QStringLiteral("inputTokens")).toDouble());
    outputTokens = static_cast<qint64>(object.value(QStringLiteral("outputTokens")).toDouble());
}
}

CodexSession::CodexSession(CodexRpcClient& rpc, QObject* parent)
    : QObject(parent), rpc_(rpc) {
    connect(&rpc_, &CodexRpcClient::notificationReceived, this,
        [this](const QString& method, const QJsonObject& params) {
            if (method == QStringLiteral("item/agentMessage/delta")) {
                const QString itemId = params.value(QStringLiteral("itemId")).toString();
                if (!itemId.isEmpty() && itemId != agentMessageItemId_) {
                    if (!agentMessageItemId_.isEmpty()) emit textDelta(QStringLiteral("\n\n"));
                    agentMessageItemId_ = itemId;
                }
                emit textDelta(params.value(QStringLiteral("delta")).toString());
            } else if (method == QStringLiteral("item/started")) {
                const QJsonObject item = params.value(QStringLiteral("item")).toObject();
                emit itemStarted(item.value(QStringLiteral("id")).toString(), item);
            } else if (method == QStringLiteral("item/completed")) {
                const QJsonObject item = params.value(QStringLiteral("item")).toObject();
                emit itemCompleted(item.value(QStringLiteral("id")).toString(), item);
            } else if (method == QStringLiteral("thread/tokenUsage/updated")) {
                const QJsonObject info = params.value(QStringLiteral("tokenUsage")).toObject();
                QJsonObject usage = info.value(QStringLiteral("last")).toObject();
                if (usage.isEmpty()) {
                    usage = info.value(QStringLiteral("total")).toObject();
                }
                if (usage.isEmpty()) usage = info;
                qint64 inputTokens = 0;
                qint64 outputTokens = 0;
                readTokenUsage(usage, inputTokens, outputTokens);
                emit tokenUsage(inputTokens, outputTokens);
            } else if (method == QStringLiteral("error")) {
                const QJsonObject error = params.value(QStringLiteral("error")).toObject();
                const QString message = error.value(QStringLiteral("message")).toString();
                qWarning() << "Stutter: Codex error:" << message
                           << "retry:" << params.value(QStringLiteral("willRetry")).toBool();
                emit turnError(message);
            } else if (method == QStringLiteral("turn/completed")) {
                const QJsonObject turn = params.value(QStringLiteral("turn")).toObject();
                const QJsonObject usage = turn.value(QStringLiteral("usage")).toObject();
                if (!usage.isEmpty()) {
                    qint64 inputTokens = 0;
                    qint64 outputTokens = 0;
                    readTokenUsage(usage, inputTokens, outputTokens);
                    emit tokenUsage(inputTokens, outputTokens);
                }
                const QString status = turn.value(QStringLiteral("status")).toString();
                if (status != QStringLiteral("completed")) {
                    qWarning() << "Stutter: Codex turn status:" << status;
                }
                emit turnCompleted(status);
            }
        });
}

void CodexSession::startThread(
    const QString& model,
    const QString& workingDirectory,
    const QString& developerInstructions
) {
    agentMessageItemId_.clear();
    QJsonObject params {
        {QStringLiteral("model"), model},
        {QStringLiteral("approvalPolicy"), QStringLiteral("never")},
        {QStringLiteral("sandbox"), QStringLiteral("read-only")},
        {QStringLiteral("serviceName"), QStringLiteral("stutter")}
    };
    if (!workingDirectory.isEmpty()) params.insert(QStringLiteral("cwd"), workingDirectory);
    if (!developerInstructions.isEmpty()) {
        params.insert(QStringLiteral("baseInstructions"), developerInstructions);
    }
    rpc_.request(QStringLiteral("thread/start"), params,
        [this](const QJsonObject& result, const QJsonObject& error) {
            if (!error.isEmpty()) {
                emit errorOccurred(error.value(QStringLiteral("message")).toString());
                return;
            }
            threadId_ = result.value(QStringLiteral("thread")).toObject()
                .value(QStringLiteral("id")).toString();
            emit threadStarted(threadId_);
        });
}

void CodexSession::startTurn(const QString& text, const QString& effort) {
    if (threadId_.isEmpty()) {
        emit errorOccurred(tr("Start a Codex thread before starting a turn"));
        return;
    }
    agentMessageItemId_.clear();
    QJsonObject params {
        {QStringLiteral("threadId"), threadId_},
        {QStringLiteral("input"), QJsonArray {QJsonObject {
            {QStringLiteral("type"), QStringLiteral("text")},
            {QStringLiteral("text"), text}
        }}}
    };
    if (!effort.isEmpty()) params.insert(QStringLiteral("effort"), effort);
    rpc_.request(
        QStringLiteral("turn/start"),
        params,
        [this](const QJsonObject& result, const QJsonObject& error) {
            if (!error.isEmpty()) {
                emit errorOccurred(error.value(QStringLiteral("message")).toString());
                return;
            }
            turnId_ = result.value(QStringLiteral("turn")).toObject()
                .value(QStringLiteral("id")).toString();
            emit turnStarted(turnId_);
        }
    );
}

void CodexSession::interrupt() {
    if (threadId_.isEmpty() || turnId_.isEmpty()) return;
    rpc_.request(
        QStringLiteral("turn/interrupt"),
        QJsonObject {
            {QStringLiteral("threadId"), threadId_},
            {QStringLiteral("turnId"), turnId_}
        }
    );
}
