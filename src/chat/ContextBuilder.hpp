#pragma once

#include <domain/Message.hpp>
#include <domain/Model.hpp>

#include <QString>
#include <QVector>

struct ChatContext {
    QString summary;
    QString analysisContext;
    QVector<stutter::Message> messages;
    bool historyTruncated = false;
};

class ContextBuilder {
public:
    ChatContext build(
        const QVector<stutter::Message>& messages,
        const QString& summary,
        const QString& analysisContext,
        const stutter::Model& model
    ) const;

    static qint64 estimateTokens(const QString& text);
};
