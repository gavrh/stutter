#pragma once

namespace stutter {

class Database;
class BinaryRepository;
class ConversationRepository;

void runStorageMaintenance(
    Database& database,
    BinaryRepository& binaries,
    ConversationRepository& conversations
);

}
