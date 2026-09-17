#pragma once

#include <config/ModelCatalog.hpp>
#include <providers/CodexProvider.hpp>
#include <storage/AnalysisNoteRepository.hpp>
#include <storage/BinaryRepository.hpp>
#include <storage/ConversationRepository.hpp>
#include <storage/Database.hpp>
#include <storage/MessageRepository.hpp>
#include <storage/SettingsRepository.hpp>

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
    stutter::Database database_;
    stutter::BinaryRepository binaryRepository_ {database_};
    stutter::ConversationRepository conversationRepository_ {database_};
    stutter::MessageRepository messageRepository_ {database_};
    stutter::AnalysisNoteRepository analysisNoteRepository_ {database_};
    stutter::SettingsRepository settingsRepository_ {database_};
    ChatController* chatController_ = nullptr;
    ChatWidget* chatWidget_ = nullptr;
    SettingsDialog* settingsDialog_ = nullptr;
};
