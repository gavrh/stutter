#include <plugin/StutterContext.hpp>

#include <chat/ChatController.hpp>
#include <domain/BinaryIdentity.hpp>
#include <ui/AnalysisContextWidget.hpp>
#include <ui/ChatWidget.hpp>
#include <ui/SettingsDialog.hpp>

#include <MainWindow.h>

#include <QFileInfo>

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
        [mainWindow]() -> stutter::BinaryIdentity {
            return stutter::BinaryRepository::identityFromPath(mainWindow->getFilename());
        },
        [this, mainWindow]() -> QString {
            return cutterGateway_.analysisContext(mainWindow->getFilename());
        },
        this
    );

    connect(&cutterGateway_, &CutterGateway::contextChanged, this, [this, mainWindow] {
        updateAnalysisContext(mainWindow);
    });
    updateAnalysisContext(mainWindow);

    connect(chatWidget_, &ChatWidget::settingsRequested, settingsDialog_, [this] {
        settingsDialog_->show();
        settingsDialog_->raise();
        settingsDialog_->activateWindow();
    });
    return chatWidget_;
}

void StutterContext::updateAnalysisContext(MainWindow* mainWindow) {
    if (!chatWidget_ || !mainWindow) return;

    const QString path = mainWindow->getFilename();
    if (path.isEmpty()) {
        chatWidget_->analysisContextWidget()->clearAnalysisContext();
        return;
    }

    const RVA address = cutterGateway_.reader().currentAddress();
    QString function = cutterGateway_.reader().functionName(address);
    if (function.isEmpty()) function = tr("No function");
    chatWidget_->analysisContextWidget()->setAnalysisContext(
        QFileInfo(path).fileName(),
        function,
        address
    );
}
