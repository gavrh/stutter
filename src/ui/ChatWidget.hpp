#pragma once

#include <CutterPlugin.h>

class AnalysisContextWidget;
class ChatMessageWidget;
class QLabel;
class MainWindow;
class QPushButton;
class QScrollArea;
class QTextEdit;
class QToolButton;
class QVBoxLayout;

class ChatWidget final : public CutterDockWidget {
    Q_OBJECT

public:
    explicit ChatWidget(MainWindow* mainWindow);

    AnalysisContextWidget* analysisContextWidget() const { return analysisContext_; }
    ChatMessageWidget* addUserMessage(const QString& content);
    ChatMessageWidget* beginAssistantMessage();
    void appendAssistantDelta(const QString& delta);
    void finishAssistantMessage();
    ChatMessageWidget* addErrorMessage(const QString& content);
    ChatMessageWidget* addToolMessage(const QString& toolName, const QString& content);
    void setBusy(bool busy);
    void setUsageText(const QString& text);
    void clearMessages();

signals:
    void messageSubmitted(const QString& message);
    void stopRequested();
    void settingsRequested();
    void conversationCleared();

private:
    ChatMessageWidget* addMessage(int kind, const QString& content);
    void removeMessage(ChatMessageWidget* message);
    void submitInput();
    bool isNearBottom() const;

    AnalysisContextWidget* analysisContext_;
    QScrollArea* messageScroll_;
    QWidget* messageContainer_;
    QVBoxLayout* messageLayout_;
    QTextEdit* input_;
    QPushButton* sendButton_;
    QPushButton* stopButton_;
    QLabel* usageLabel_;
    ChatMessageWidget* streamingMessage_ = nullptr;
    bool busy_ = false;
    bool followStreaming_ = false;
    bool adjustingScroll_ = false;
};
