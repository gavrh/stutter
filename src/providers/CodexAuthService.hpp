#pragma once

#include <QObject>
#include <QString>

class CodexProcess;
class CodexRpcClient;

class CodexAuthService final : public QObject {
    Q_OBJECT

public:
    explicit CodexAuthService(
        CodexProcess& process,
        CodexRpcClient& rpc,
        QObject* parent = nullptr
    );

    bool isAvailable() const;
    bool isConnected() const { return connected_; }
    QString statusText() const { return statusText_; }
    void connectChatGpt();
    void disconnect();
    void refresh();
    void logout();

signals:
    void statusChanged(const QString& text, bool connected, bool busy);

private:
    void readAccount(bool loginIfMissing);
    void beginLogin();
    void setStatus(const QString& text, bool connected, bool busy = false);

    CodexProcess& process_;
    CodexRpcClient& rpc_;
    QString loginId_;
    QString statusText_;
    bool connected_ = false;
    bool loginRequested_ = false;
};
