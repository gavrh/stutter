#include <ui/ChatMessageWidget.hpp>

#include <QAbstractTextDocumentLayout>
#include <QFrame>
#include <QLabel>
#include <QTextBrowser>
#include <QTextDocument>
#include <QVBoxLayout>
#include <QtMath>

namespace {
QString titleFor(ChatMessageKind kind) {
    switch (kind) {
    case ChatMessageKind::User: return QStringLiteral("You");
    case ChatMessageKind::Assistant: return QStringLiteral("Stutter");
    case ChatMessageKind::Error: return QStringLiteral("Error");
    case ChatMessageKind::Tool: return QStringLiteral("Tool");
    }
    return {};
}
}

ChatMessageWidget::ChatMessageWidget(
    ChatMessageKind kind,
    const QString& content,
    QWidget* parent
) : QWidget(parent), kind_(kind), content_(content) {
    setObjectName(QStringLiteral("chatMessage"));
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 8, 10, 8);
    layout->setSpacing(4);

    roleLabel_ = new QLabel(titleFor(kind), this);
    QFont titleFont = roleLabel_->font();
    titleFont.setBold(true);
    roleLabel_->setFont(titleFont);

    contentView_ = new QTextBrowser(this);
    contentView_->setFrameShape(QFrame::NoFrame);
    contentView_->setOpenExternalLinks(true);
    contentView_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    contentView_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    contentView_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    contentView_->document()->setDocumentMargin(0);

    layout->addWidget(roleLabel_);
    layout->addWidget(contentView_);
    connect(
        contentView_->document()->documentLayout(),
        &QAbstractTextDocumentLayout::documentSizeChanged,
        this,
        [this](const QSizeF& size) {
            contentView_->setFixedHeight(qMax(24, qCeil(size.height()) + 2));
        }
    );
    render();
}

void ChatMessageWidget::setContent(const QString& content) {
    content_ = content;
    render();
}

void ChatMessageWidget::appendContent(const QString& content) {
    content_.append(content);
    render();
}

void ChatMessageWidget::render() {
    if (kind_ == ChatMessageKind::Assistant) {
        contentView_->setMarkdown(content_);
    } else {
        contentView_->setPlainText(content_);
    }
    contentView_->document()->adjustSize();
}
