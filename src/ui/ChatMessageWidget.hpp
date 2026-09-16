#pragma once

#include <QWidget>

class QLabel;
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
    void setContent(const QString& content);
    void appendContent(const QString& content);

private:
    void render();

    ChatMessageKind kind_;
    QString content_;
    QLabel* roleLabel_;
    QTextBrowser* contentView_;
};
