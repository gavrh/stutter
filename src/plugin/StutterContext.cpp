#include <plugin/StutterContext.hpp>

#include <chat/ChatController.hpp>
#include <domain/BinaryIdentity.hpp>
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
        database_.applyMigrations();
    }

    settingsDialog_ = new SettingsDialog(
        modelCatalog_,
        codexProvider_,
        &settingsRepository_,
        mainWindow
    );
    chatWidget_ = new ChatWidget(mainWindow);
    chatController_ = new ChatController(
        *chatWidget_,
        *settingsDialog_,
        modelCatalog_,
        codexProvider_,
        binaryRepository_,
        conversationRepository_,
        messageRepository_,
        [mainWindow]() -> stutter::BinaryIdentity {
            return stutter::BinaryRepository::identityFromPath(mainWindow->getFilename());
        },
        this
    );

    connect(chatWidget_, &ChatWidget::settingsRequested, settingsDialog_, [this] {
        settingsDialog_->show();
        settingsDialog_->raise();
        settingsDialog_->activateWindow();
    });
    return chatWidget_;
}
