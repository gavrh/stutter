#include <storage/StorageMaintenance.hpp>

#include <domain/BinaryIdentity.hpp>
#include <storage/BinaryRepository.hpp>
#include <storage/ConversationRepository.hpp>
#include <storage/Database.hpp>

#include <QDateTime>
#include <QFileInfo>

namespace stutter {

void runStorageMaintenance(
    Database& database,
    BinaryRepository& binaries,
    ConversationRepository& conversations
) {
    if (!database.isOpen()) {
        return;
    }

    const QDateTime cutoff = QDateTime::currentDateTimeUtc().addMonths(-3);
    for (const BinaryIdentity& binary : binaries.all()) {
        QDateTime lastActivity = conversations.latestUpdatedAt(binary.id);
        if (!lastActivity.isValid()) lastActivity = binary.lastSeenAt;
        if (!lastActivity.isValid()) lastActivity = binary.updatedAt;

        const bool fileExists = !binary.path.isEmpty() && QFileInfo::exists(binary.path);
        if (!fileExists) {
            conversations.removeForBinary(binary.id);
        }

        if (lastActivity.isValid() && lastActivity < cutoff) {
            conversations.removeForBinary(binary.id);
            binaries.remove(binary.id);
        }
    }
}

}
