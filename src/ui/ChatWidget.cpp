#include <ui/ChatWidget.hpp>

#include <ui/AnalysisContextWidget.hpp>
#include <ui/ChatMessageWidget.hpp>

#include <constants.h>
#include <MainWindow.h>

#include <QAbstractTextDocumentLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QStringList>
#include <QTextEdit>
#include <QTextDocument>
#include <QToolButton>
#include <QVBoxLayout>
#include <QtMath>

#include <functional>

static QString truncateLines(const QString& text, int maxLines) {
    const QStringList lines = text.split(QLatin1Char('\n'));
    if (lines.size() <= maxLines) return text;
    const int hidden = lines.size() - maxLines;
    return lines.mid(0, maxLines).join(QLatin1Char('\n'))
        + QStringLiteral("\n... +%1 lines").arg(hidden);
}

class ChatInput final : public QTextEdit {
public:
    explicit ChatInput(QWidget* parent = nullptr) : QTextEdit(parent) {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        connect(
            document()->documentLayout(),
            &QAbstractTextDocumentLayout::documentSizeChanged,
            this,
            [this](const QSizeF&) { updateGeometry(); }
        );
    }

    std::function<void()> submit;

    QSize sizeHint() const override {
        QSize hint = QTextEdit::sizeHint();
        hint.setHeight(qBound(minimumInputHeight, documentHeight(), maximumInputHeight));
        return hint;
    }

    QSize minimumSizeHint() const override {
        QSize hint = QTextEdit::minimumSizeHint();
        hint.setHeight(minimumInputHeight);
        return hint;
    }

protected:
    void keyPressEvent(QKeyEvent* event) override {
        const bool enter = event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter;
        if (enter && !(event->modifiers() & Qt::ShiftModifier)) {
            if (submit) submit();
            event->accept();
            return;
        }
        QTextEdit::keyPressEvent(event);
    }

private:
    int documentHeight() const {
        return qCeil(document()->documentLayout()->documentSize().height())
            + 2 * frameWidth();
    }

    static constexpr int minimumInputHeight = 64;
    static constexpr int maximumInputHeight = 160;
};

ChatWidget::ChatWidget(MainWindow* mainWindow) : CutterDockWidget(mainWindow) {
    setObjectName(QStringLiteral("StutterChat"));
    setWindowTitle(QString(STUTTER_DISPLAY_NAME.data()));

    auto* content = new QWidget(this);
    auto* root = new QVBoxLayout(content);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(8);

    auto* controls = new QHBoxLayout;
    auto* clearButton = new QToolButton(content);
    clearButton->setText(tr("Clear"));
    clearButton->setToolTip(tr("Clear displayed messages"));
    auto* settingsButton = new QToolButton(content);
    settingsButton->setText(tr("Settings"));
    sendButton_ = new QPushButton(tr("Send"), content);
    stopButton_ = new QPushButton(tr("Stop"), content);
    stopButton_->hide();
    controls->addWidget(clearButton);
    controls->addWidget(settingsButton);
    controls->addStretch();
    controls->addWidget(sendButton_);
    controls->addWidget(stopButton_);

    analysisContext_ = new AnalysisContextWidget(content);

    messageScroll_ = new QScrollArea(content);
    messageScroll_->setWidgetResizable(true);
    messageScroll_->setFrameShape(QFrame::NoFrame);
    messageScroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    messageContainer_ = new QWidget(messageScroll_);
    messageLayout_ = new QVBoxLayout(messageContainer_);
    messageLayout_->setContentsMargins(0, 0, 0, 0);
    messageLayout_->setSpacing(16);
    messageLayout_->setAlignment(Qt::AlignBottom);
    messageScroll_->setWidget(messageContainer_);
    QScrollBar* scrollBar = messageScroll_->verticalScrollBar();
    connect(scrollBar, &QScrollBar::valueChanged, this, [this](int) {
        if (adjustingScroll_) return;
        followStreaming_ = isNearBottom();
    });
    connect(scrollBar, &QScrollBar::rangeChanged, this, [this](int, int) {
        if (!followStreaming_) return;
        adjustingScroll_ = true;
        QScrollBar* bar = messageScroll_->verticalScrollBar();
        bar->setValue(bar->maximum());
        adjustingScroll_ = false;
    });

    auto* chatInput = new ChatInput(content);
    input_ = chatInput;
    input_->setAcceptRichText(false);
    input_->setPlaceholderText(tr("Ask about the current binary, Shift+Enter adds a line"));

    auto* footer = new QHBoxLayout;
    usageLabel_ = new QLabel(tr("No usage data"), content);
    auto* version = new QLabel(QString(STUTTER_VERSION_STR.data()), content);
    footer->addWidget(usageLabel_);
    footer->addStretch();
    footer->addWidget(version);

    root->addWidget(analysisContext_);
    root->addWidget(messageScroll_, 1);
    root->addLayout(controls);
    root->addWidget(input_);
    root->addLayout(footer);
    setWidget(content);

    chatInput->submit = [this] { submitInput(); };
    connect(sendButton_, &QPushButton::clicked, this, &ChatWidget::submitInput);
    connect(stopButton_, &QPushButton::clicked, this, &ChatWidget::stopRequested);
    connect(settingsButton, &QToolButton::clicked, this, &ChatWidget::settingsRequested);
    connect(clearButton, &QToolButton::clicked, this, [this] {
        clearMessages();
        emit conversationCleared();
    });
}

ChatMessageWidget* ChatWidget::addMessage(int kind, const QString& content) {
    auto* message = new ChatMessageWidget(static_cast<ChatMessageKind>(kind), content, messageContainer_);
    messageLayout_->addWidget(message);
    return message;
}

void ChatWidget::removeMessage(ChatMessageWidget* message) {
    messageLayout_->removeWidget(message);
    message->deleteLater();
}

ChatMessageWidget* ChatWidget::addUserMessage(const QString& content) {
    followStreaming_ = true;
    return addMessage(static_cast<int>(ChatMessageKind::User), content);
}

ChatMessageWidget* ChatWidget::addAssistantMessage(const QString& content) {
    return addMessage(static_cast<int>(ChatMessageKind::Assistant), content);
}

ChatMessageWidget* ChatWidget::beginAssistantMessage() {
    if (!streamingMessage_) {
        streamingMessage_ = addMessage(static_cast<int>(ChatMessageKind::Assistant), {});
    }
    return streamingMessage_;
}

void ChatWidget::appendAssistantDelta(const QString& delta) {
    beginAssistantMessage()->appendContent(delta);
}

void ChatWidget::finishAssistantMessage() {
    streamingMessage_ = nullptr;
    setBusy(false);
}

ChatMessageWidget* ChatWidget::addErrorMessage(const QString& content) {
    if (streamingMessage_ && streamingMessage_->content().isEmpty()) {
        removeMessage(streamingMessage_);
    }
    streamingMessage_ = nullptr;
    setBusy(false);
    return addMessage(static_cast<int>(ChatMessageKind::Error), content);
}

ChatMessageWidget* ChatWidget::addToolMessage(const QString& toolName, const QString& content) {
    return addMessage(
        static_cast<int>(ChatMessageKind::Tool),
        QStringLiteral("%1\n%2").arg(toolName, content)
    );
}

ChatMessageWidget* ChatWidget::addActivity(const stutter::ToolActivity& activity) {
    stutter::ToolActivity entry = activity;
    entry.detail = truncateLines(entry.detail, 5);
    entry.result = truncateLines(entry.result, 5);
    auto* message = beginAssistantMessage();
    message->setActivity(entry);
    return message;
}

void ChatWidget::updateActivity(const stutter::ToolActivity& activity) {
    addActivity(activity);
}

void ChatWidget::setBusy(bool busy) {
    busy_ = busy;
    sendButton_->setVisible(!busy);
    stopButton_->setVisible(busy);
}

void ChatWidget::setUsageText(const QString& text) {
    usageLabel_->setText(text.isEmpty() ? tr("No usage data") : text);
}

void ChatWidget::clearMessages() {
    streamingMessage_ = nullptr;
    while (messageLayout_->count() > 0) {
        QLayoutItem* item = messageLayout_->takeAt(0);
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
    followStreaming_ = false;
}

void ChatWidget::submitInput() {
    const QString message = input_->toPlainText().trimmed();
    if (message.isEmpty() || busy_) return;
    input_->clear();
    emit messageSubmitted(message);
}

bool ChatWidget::isNearBottom() const {
    QScrollBar* bar = messageScroll_->verticalScrollBar();
    const int margin = qMax(24, fontMetrics().height() * 2);
    return bar->maximum() - bar->value() <= margin;
}
