#pragma once

#include <QWidget>

class QLabel;
class QResizeEvent;
class QTextBrowser;

enum class ChatMessageKind {
    User,
    Assistant,
    Error,
    Tool
};

class ChatMessageWidget final : public QWidget {
    Q_OBJECT

public:
    explicit ChatMessageWidget(
        ChatMessageKind kind,
        const QString& content = {},
        QWidget* parent = nullptr
    );

    ChatMessageKind kind() const { return kind_; }
    QString content() const { return content_; }
    void setTitle(const QString& title);
    void setContent(const QString& content);
    void appendContent(const QString& content);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void render();
    void updateDocumentWidth();
    void updateContentHeight();

    ChatMessageKind kind_;
    QString content_;
    QLabel* roleLabel_;
    QLabel* contentLabel_ = nullptr;
    QTextBrowser* contentBrowser_ = nullptr;
};
