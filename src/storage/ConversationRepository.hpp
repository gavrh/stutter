#pragma once

#include <domain/Conversation.hpp>

#include <QString>
#include <QVector>

namespace stutter {

class Database;

class ConversationRepository {
public:
    explicit ConversationRepository(Database& database);

    QVector<stutter::Conversation> forBinary(const QString& binaryId);
    stutter::Conversation latestForBinary(const QString& binaryId);

    bool save(const stutter::Conversation& conversation);
    bool remove(const QString& conversationId);
    bool updateSummary(const QString& conversationId, const QString& summary);

private:
    Database& database_;
};

}
