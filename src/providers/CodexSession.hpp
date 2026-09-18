#pragma once

#include <QObject>
#include <QString>

class CodexRpcClient;

class CodexSession final : public QObject {
    Q_OBJECT

public:
    explicit CodexSession(CodexRpcClient& rpc, QObject* parent = nullptr);

    void startThread(
        const QString& model,
        const QString& workingDirectory,
        const QString& developerInstructions = {}
    );
    void startTurn(const QString& text, const QString& effort = {});
    void interrupt();
    QString threadId() const { return threadId_; }

signals:
    void threadStarted(const QString& threadId);
    void turnStarted(const QString& turnId);
    void textDelta(const QString& delta);
    void tokenUsage(qint64 inputTokens, qint64 outputTokens);
    void turnCompleted(const QString& status);
    void errorOccurred(const QString& message);

private:
    CodexRpcClient& rpc_;
    QString threadId_;
    QString turnId_;
    QString agentMessageItemId_;
};
