#pragma once

#include <config/ModelCatalog.hpp>
#include <providers/CodexProvider.hpp>

#include <QObject>

class ChatWidget;
class ChatController;
class MainWindow;
class SettingsDialog;

class StutterContext final : public QObject {
    Q_OBJECT

public:
    explicit StutterContext(QObject* parent = nullptr);

    ChatWidget* createChatWidget(MainWindow* mainWindow);
    ChatWidget* chatWidget() const { return chatWidget_; }
    SettingsDialog* settingsDialog() const { return settingsDialog_; }
    CodexProvider* codexProvider() { return &codexProvider_; }

private:
    ModelCatalog modelCatalog_;
    CodexProvider codexProvider_;
    ChatController* chatController_ = nullptr;
    ChatWidget* chatWidget_ = nullptr;
    SettingsDialog* settingsDialog_ = nullptr;
};
