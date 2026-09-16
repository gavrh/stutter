#pragma once

#include <domain/Message.hpp>
#include <providers/ProviderTypes.hpp>

#include <QString>
#include <QVector>

class HistorySummarizer {
public:
    HistorySummarizer();

    bool shouldSummarize(const QVector<stutter::Message>& messages, qint64 tokenBudget) const;
    QVector<stutter::Message> messagesForSummary(
        const QVector<stutter::Message>& messages,
        qint64 targetTokens
    ) const;
    ChatRequest buildRequest(
        const QVector<stutter::Message>& messages,
        const QString& model
    ) const;
    QString error() const { return error_; }

private:
    QString prompt_;
    QString error_;
};
