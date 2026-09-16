#pragma once

#include <QObject>

class ChatWidget;
class MainWindow;
class SettingsDialog;

class StutterContext final : public QObject {
    Q_OBJECT

public:
    explicit StutterContext(QObject* parent = nullptr);

    ChatWidget* createChatWidget(MainWindow* mainWindow);
    ChatWidget* chatWidget() const { return chatWidget_; }
    SettingsDialog* settingsDialog() const { return settingsDialog_; }

private:
    ChatWidget* chatWidget_ = nullptr;
    SettingsDialog* settingsDialog_ = nullptr;
};
