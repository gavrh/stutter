#include <providers/CodexRpcClient.hpp>

#include <providers/CodexProcess.hpp>

#include <constants.h>

#include <QJsonDocument>

CodexRpcClient::CodexRpcClient(CodexProcess& process, QObject* parent)
    : QObject(parent), process_(process) {
    connect(&process_, &CodexProcess::started, this, &CodexRpcClient::initialize);
    connect(&process_, &CodexProcess::standardOutput, this, &CodexRpcClient::consume);
    connect(&process_, &CodexProcess::stopped, this, [this] {
        ready_ = false;
        pending_.clear();
    });
    connect(&process_, &CodexProcess::errorOccurred, this, &CodexRpcClient::protocolError);
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
    if (handler) pending_.insert(id, std::move(handler));
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
        const ReplyHandler handler = pending_.take(id);
        if (handler) {
            handler(
                message.value(QStringLiteral("result")).toObject(),
                message.value(QStringLiteral("error")).toObject()
            );
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

void CodexRpcClient::send(const QJsonObject& message) {
    QByteArray bytes = QJsonDocument(message).toJson(QJsonDocument::Compact);
    bytes.append('\n');
    if (process_.write(bytes) < 0) emit protocolError(tr("Codex app-server is not running"));
}
