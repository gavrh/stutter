#pragma once

#include <domain/Message.hpp>

#include <QString>
#include <QVector>

namespace stutter {

class Database;

class MessageRepository {
public:
    explicit MessageRepository(Database& database);

    QVector<stutter::Message> forConversation(const QString& conversationId);
    bool save(const stutter::Message& message);
    bool removeForConversation(const QString& conversationId);

private:
    Database& database_;
};

}
