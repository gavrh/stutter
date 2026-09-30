#include <plugin/StutterContext.hpp>

#include <chat/ChatController.hpp>
#include <domain/BinaryIdentity.hpp>
#include <storage/StorageMaintenance.hpp>
#include <ui/ChatWidget.hpp>
#include <ui/SettingsDialog.hpp>

#include <MainWindow.h>

StutterContext::StutterContext(QObject* parent)
    : QObject(parent),
      codexProvider_(this) {}

ChatWidget* StutterContext::createChatWidget(MainWindow* mainWindow) {
    if (chatWidget_) {
        return chatWidget_;
    }

    if (!database_.isOpen()) {
        database_.open(stutter::Database::defaultDatabasePath());
        if (database_.applyMigrations()) {
            stutter::runStorageMaintenance(database_, binaryRepository_, conversationRepository_);
        }
    }

    settingsDialog_ = new SettingsDialog(
        modelCatalog_,
        codexProvider_,
        &settingsRepository_,
        mainWindow
    );
    chatWidget_ = new ChatWidget(mainWindow);
    stutter::registerAllTools(toolRegistry_, cutterGateway_);
    toolExecutor_ = std::make_unique<stutter::ToolExecutor>(
        toolRegistry_,
        [this](stutter::ToolPermission permission) {
            switch (permission) {
            case stutter::ToolPermission::Read: return true;
            case stutter::ToolPermission::Analysis:
                return settingsDialog_->allowAnalysisChanges();
            case stutter::ToolPermission::Binary:
                return settingsDialog_->allowBinaryChanges();
            case stutter::ToolPermission::Debugger:
                return settingsDialog_->allowDebuggerControl();
            }
            return false;
        }
    );
    chatController_ = new ChatController(
        *chatWidget_,
        *settingsDialog_,
        modelCatalog_,
        codexProvider_,
        binaryRepository_,
        conversationRepository_,
        messageRepository_,
        toolRegistry_,
        *toolExecutor_,
        [this, mainWindow]() -> stutter::BinaryIdentity {
            return binaryRepository_.identityFromPath(mainWindow->getFilename());
        },
        [this, mainWindow]() -> QString {
            return cutterGateway_.analysisContext(mainWindow->getFilename());
        },
        this
    );

    connect(&cutterGateway_, &CutterGateway::contextChanged, this, [this, mainWindow] {
        refreshConversation(mainWindow);
    });
    refreshConversation(mainWindow);

    connect(chatWidget_, &ChatWidget::settingsRequested, settingsDialog_, [this] {
        settingsDialog_->show();
        settingsDialog_->raise();
        settingsDialog_->activateWindow();
    });
    return chatWidget_;
}

void StutterContext::refreshConversation(MainWindow* mainWindow) {
    if (!chatWidget_ || !mainWindow) return;
    if (mainWindow->getFilename().isEmpty()) return;
    if (chatController_) {
        chatController_->refreshConversation();
    }
}
