#pragma once

#include <providers/Provider.hpp>

#include <QNetworkAccessManager>
#include <QUrl>

#include <memory>
#include <unordered_map>

class QNetworkReply;

class OpenAIProvider final : public Provider {
    Q_OBJECT

public:
    explicit OpenAIProvider(
        QString apiKey,
        QUrl baseUrl = {},
        bool includeStreamUsage = true,
        QObject* parent = nullptr
    );
    ~OpenAIProvider() override;

    ProviderType type() const override { return ProviderType::OpenAI; }
    QString name() const override { return QStringLiteral("OpenAI"); }
    QString send(const ChatRequest& request) override;
    void cancel(const QString& requestId) override;

private:
    struct RequestState;

    void readReply(QNetworkReply* reply);
    void finishReply(QNetworkReply* reply);
    void processEvents(RequestState& state, QVector<ProviderEvent> events);

    QString apiKey_;
    QUrl baseUrl_;
    bool includeStreamUsage_ = true;
    QNetworkAccessManager network_;
    std::unordered_map<QNetworkReply*, std::unique_ptr<RequestState>> requests_;
};
