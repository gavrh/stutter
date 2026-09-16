#include <plugin/StutterContext.hpp>

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

    settingsDialog_ = new SettingsDialog(modelCatalog_, codexProvider_, mainWindow);
    chatWidget_ = new ChatWidget(mainWindow);

    connect(chatWidget_, &ChatWidget::settingsRequested, settingsDialog_, [this] {
        settingsDialog_->show();
        settingsDialog_->raise();
        settingsDialog_->activateWindow();
    });
    return chatWidget_;
}
