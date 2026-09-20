#pragma once

#include <domain/ToolActivity.hpp>

#include <QHash>
#include <QVector>
#include <QWidget>

class ActivityBlock;
class QLabel;
class QResizeEvent;
class QTextBrowser;
class QVBoxLayout;

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
    void setActivity(const stutter::ToolActivity& activity);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void render();
    void addTextLabel(const QString& text);
    ActivityBlock* createActivityBlock();
    void updateTextLabelHeight(QLabel* label);
    void updateBrowserWidth(QTextBrowser* browser);

    ChatMessageKind kind_;
    QString content_;
    QString segmentText_;
    QLabel* roleLabel_;
    QLabel* contentLabel_ = nullptr;
    QTextBrowser* contentBrowser_ = nullptr;
    QVBoxLayout* bodyLayout_ = nullptr;
    QLabel* activeTextLabel_ = nullptr;
    QVector<QLabel*> textLabels_;
    QHash<QString, ActivityBlock*> activityBlocks_;
};
