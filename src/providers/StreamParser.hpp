#pragma once

#include <providers/ProviderTypes.hpp>

#include <QByteArray>
#include <QHash>
#include <QVector>

class StreamParser {
public:
    virtual ~StreamParser() = default;
    virtual QVector<ProviderEvent> push(const QByteArray& bytes) = 0;
    virtual QVector<ProviderEvent> finish() = 0;
};

class OpenAIStreamParser final : public StreamParser {
public:
    QVector<ProviderEvent> push(const QByteArray& bytes) override;
    QVector<ProviderEvent> finish() override;

private:
    QVector<ProviderEvent> consume(bool flush);
    QVector<ProviderEvent> parseData(const QByteArray& data);

    QByteArray buffer_;
    TokenUsage usage_;
    QString stopReason_;
    bool completed_ = false;
};

class AnthropicStreamParser final : public StreamParser {
public:
    QVector<ProviderEvent> push(const QByteArray& bytes) override;
    QVector<ProviderEvent> finish() override;

private:
    QVector<ProviderEvent> consume(bool flush);
    QVector<ProviderEvent> parseData(const QByteArray& data);

    QByteArray buffer_;
    QHash<int, QString> toolIds_;
    QHash<int, QString> toolNames_;
    TokenUsage usage_;
    QString stopReason_;
    bool completed_ = false;
};
