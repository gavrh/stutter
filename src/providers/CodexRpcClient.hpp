#pragma once

#include <QHash>
#include <QJsonObject>
#include <QObject>

#include <functional>

class CodexProcess;

class CodexRpcClient final : public QObject {
    Q_OBJECT

public:
    using ReplyHandler = std::function<void(const QJsonObject& result, const QJsonObject& error)>;

    explicit CodexRpcClient(CodexProcess& process, QObject* parent = nullptr);

    bool isReady() const { return ready_; }
    void start();
    qint64 request(const QString& method, const QJsonObject& params, ReplyHandler handler = {});
    void notify(const QString& method, const QJsonObject& params = {});

signals:
    void ready();
    void notificationReceived(const QString& method, const QJsonObject& params);
    void protocolError(const QString& message);

private:
    void initialize();
    void consume(const QByteArray& bytes);
    void handleMessage(const QJsonObject& message);
    void send(const QJsonObject& message);

    CodexProcess& process_;
    QByteArray buffer_;
    QHash<qint64, ReplyHandler> pending_;
    qint64 nextId_ = 1;
    bool ready_ = false;
};
