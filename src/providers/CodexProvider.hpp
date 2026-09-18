#pragma once

#include <providers/Provider.hpp>

#include <providers/CodexAuthService.hpp>
#include <providers/CodexProcess.hpp>
#include <providers/CodexRpcClient.hpp>
#include <providers/CodexSession.hpp>

class CodexProvider final : public Provider {
    Q_OBJECT

public:
    explicit CodexProvider(QObject* parent = nullptr);

    ProviderType type() const override { return ProviderType::Codex; }
    QString name() const override { return QStringLiteral("Codex"); }
    QString send(const ChatRequest& request) override;
    void cancel(const QString& requestId) override;

    bool isAvailable() const { return auth_.isAvailable(); }
    bool isConnected() const { return auth_.isConnected(); }
    QString connectionStatus() const { return auth_.statusText(); }
    void connectChatGpt() { auth_.connectChatGpt(); }
    void disconnect();
    void logout() { auth_.logout(); }

signals:
    void connectionStatusChanged(const QString& text, bool connected, bool busy);

private:
    QString promptFor(const ChatRequest& request) const;
    QString instructionsFor(const ChatRequest& request) const;
    void failActive(const QString& message);

    CodexProcess process_;
    CodexRpcClient rpc_;
    CodexAuthService auth_;
    CodexSession session_;
    QString activeRequestId_;
    QString pendingPrompt_;
    QString pendingInstructions_;
    QString pendingEffort_;
    ChatResponse response_;
};
