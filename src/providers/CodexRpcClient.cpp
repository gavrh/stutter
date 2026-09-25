#include <providers/CodexRpcClient.hpp>

#include <providers/CodexProcess.hpp>

#include <constants.h>

#include <QDateTime>
#include <QJsonDocument>
#include <QVector>

#include <utility>

CodexRpcClient::CodexRpcClient(CodexProcess& process, QObject* parent)
    : QObject(parent), process_(process) {
    connect(&process_, &CodexProcess::started, this, &CodexRpcClient::initialize);
    connect(&process_, &CodexProcess::standardOutput, this, &CodexRpcClient::consume);
    connect(&process_, &CodexProcess::stopped, this, [this] {
        ready_ = false;
        pending_.clear();
    });
    connect(&process_, &CodexProcess::errorOccurred, this, &CodexRpcClient::protocolError);

    timeoutTimer_.setInterval(5000);
    connect(&timeoutTimer_, &QTimer::timeout, this, &CodexRpcClient::checkTimeouts);
    timeoutTimer_.start();
}

void CodexRpcClient::start() {
    if (ready_) {
        emit ready();
    } else {
        process_.start();
    }
}

qint64 CodexRpcClient::request(
    const QString& method,
    const QJsonObject& params,
    ReplyHandler handler
) {
    const qint64 id = nextId_++;
    if (handler) {
        pending_.insert(id, PendingRequest {std::move(handler), QDateTime::currentMSecsSinceEpoch()});
    }
    send(QJsonObject {
        {QStringLiteral("method"), method},
        {QStringLiteral("id"), id},
        {QStringLiteral("params"), params}
    });
    return id;
}

void CodexRpcClient::notify(const QString& method, const QJsonObject& params) {
    send(QJsonObject {{QStringLiteral("method"), method}, {QStringLiteral("params"), params}});
}

void CodexRpcClient::initialize() {
    const QJsonObject clientInfo {
        {QStringLiteral("name"), QStringLiteral("stutter")},
        {QStringLiteral("title"), QStringLiteral("Stutter")},
        {QStringLiteral("version"), QString(STUTTER_VERSION_STR.data())}
    };
    request(
        QStringLiteral("initialize"),
        QJsonObject {{QStringLiteral("clientInfo"), clientInfo}},
        [this](const QJsonObject&, const QJsonObject& error) {
            if (!error.isEmpty()) {
                emit protocolError(error.value(QStringLiteral("message")).toString());
                return;
            }
            notify(QStringLiteral("initialized"));
            ready_ = true;
            emit ready();
        }
    );
}

void CodexRpcClient::consume(const QByteArray& bytes) {
    buffer_.append(bytes);
    while (true) {
        const auto newline = buffer_.indexOf('\n');
        if (newline < 0) return;
        QByteArray line = buffer_.left(newline).trimmed();
        buffer_.remove(0, newline + 1);
        if (line.isEmpty()) continue;
        QJsonParseError error;
        const QJsonDocument document = QJsonDocument::fromJson(line, &error);
        if (error.error != QJsonParseError::NoError || !document.isObject()) {
            emit protocolError(tr("Invalid JSON from Codex app-server: %1").arg(error.errorString()));
            continue;
        }
        handleMessage(document.object());
    }
}

void CodexRpcClient::handleMessage(const QJsonObject& message) {
    if (message.contains(QStringLiteral("id")) &&
        (message.contains(QStringLiteral("result")) || message.contains(QStringLiteral("error")))) {
        const qint64 id = static_cast<qint64>(message.value(QStringLiteral("id")).toDouble());
        const auto iterator = pending_.find(id);
        if (iterator != pending_.end()) {
            ReplyHandler handler = std::move(iterator->handler);
            pending_.erase(iterator);
            if (handler) {
                handler(
                    message.value(QStringLiteral("result")).toObject(),
                    message.value(QStringLiteral("error")).toObject()
                );
            }
        }
        return;
    }
    if (!message.contains(QStringLiteral("id"))) {
        emit notificationReceived(
            message.value(QStringLiteral("method")).toString(),
            message.value(QStringLiteral("params")).toObject()
        );
        return;
    }

    send(QJsonObject {
        {QStringLiteral("id"), message.value(QStringLiteral("id"))},
        {QStringLiteral("error"), QJsonObject {
            {QStringLiteral("code"), -32601},
            {QStringLiteral("message"), QStringLiteral("Client method not implemented")}
        }}
    });
}

void CodexRpcClient::checkTimeouts() {
    constexpr qint64 requestTimeoutMs = 120000;
    const qint64 now = QDateTime::currentMSecsSinceEpoch();

    QVector<qint64> expired;
    for (auto iterator = pending_.cbegin(); iterator != pending_.cend(); ++iterator) {
        if (now - iterator.value().startedAt > requestTimeoutMs) {
            expired.append(iterator.key());
        }
    }

    for (qint64 id : expired) {
        const auto iterator = pending_.find(id);
        if (iterator == pending_.end()) continue;
        ReplyHandler handler = std::move(iterator->handler);
        pending_.erase(iterator);
        if (handler) {
            handler({}, QJsonObject {
                {QStringLiteral("code"), -32000},
                {QStringLiteral("message"), tr("Codex request timed out")}
            });
        }
        emit protocolError(tr("Codex request timed out"));
    }
}

void CodexRpcClient::send(const QJsonObject& message) {
    QByteArray bytes = QJsonDocument(message).toJson(QJsonDocument::Compact);
    bytes.append('\n');
    if (process_.write(bytes) < 0) emit protocolError(tr("Codex app-server is not running"));
}
