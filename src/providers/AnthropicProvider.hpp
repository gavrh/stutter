#pragma once

#include <providers/Provider.hpp>

#include <QNetworkAccessManager>
#include <QUrl>

#include <memory>
#include <unordered_map>

class QNetworkReply;

class AnthropicProvider final : public Provider {
    Q_OBJECT

public:
    explicit AnthropicProvider(
        QString apiKey,
        QString apiVersion = QStringLiteral("2023-06-01"),
        QUrl baseUrl = {},
        QObject* parent = nullptr
    );
    ~AnthropicProvider() override;

    ProviderType type() const override { return ProviderType::Anthropic; }
    QString name() const override { return QStringLiteral("Anthropic"); }
    QString send(const ChatRequest& request) override;
    void cancel(const QString& requestId) override;

private:
    struct RequestState;

    void readReply(QNetworkReply* reply);
    void finishReply(QNetworkReply* reply);
    void processEvents(RequestState& state, QVector<ProviderEvent> events);

    QString apiKey_;
    QString apiVersion_;
    QUrl baseUrl_;
    QNetworkAccessManager network_;
    std::unordered_map<QNetworkReply*, std::unique_ptr<RequestState>> requests_;
};
