#include <plugin/StutterContext.hpp>

#include <ui/ChatWidget.hpp>
#include <ui/SettingsDialog.hpp>

#include <MainWindow.h>

StutterContext::StutterContext(QObject* parent) : QObject(parent) {}

ChatWidget* StutterContext::createChatWidget(MainWindow* mainWindow) {
    if (chatWidget_) {
        return chatWidget_;
    }

    settingsDialog_ = new SettingsDialog(mainWindow);
    chatWidget_ = new ChatWidget(mainWindow);

    connect(chatWidget_, &ChatWidget::settingsRequested, settingsDialog_, [this] {
        settingsDialog_->show();
        settingsDialog_->raise();
        settingsDialog_->activateWindow();
    });
    return chatWidget_;
}
