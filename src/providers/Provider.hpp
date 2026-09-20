#pragma once

#include <domain/ToolActivity.hpp>
#include <providers/ProviderTypes.hpp>

#include <QObject>
#include <QString>

class Provider : public QObject {
    Q_OBJECT

public:
    explicit Provider(QObject* parent = nullptr) : QObject(parent) {
        qRegisterMetaType<ChatResponse>();
        qRegisterMetaType<ProviderError>();
        qRegisterMetaType<ProviderEvent>();
        qRegisterMetaType<stutter::ToolActivity>();
    }
    ~Provider() override = default;

    virtual ProviderType type() const = 0;
    virtual QString name() const = 0;

    // Requests are asynchronous. Signals for a request carry the returned ID.
    virtual QString send(const ChatRequest& request) = 0;
    virtual void cancel(const QString& requestId) = 0;

signals:
    void eventReceived(const ProviderEvent& event);
    void finished(const QString& requestId, const ChatResponse& response);
    void failed(const QString& requestId, const ProviderError& error);
    void activityStarted(const stutter::ToolActivity& activity);
    void activityUpdated(const stutter::ToolActivity& activity);
    void activityFinished(const stutter::ToolActivity& activity);
};
