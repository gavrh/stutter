#include <ui/ChatMessageWidget.hpp>

#include <QAbstractTextDocumentLayout>
#include <QFrame>
#include <QLabel>
#include <QResizeEvent>
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
    layout->setContentsMargins(0, 8, 0, 8);
    layout->setSpacing(6);

    roleLabel_ = new QLabel(titleFor(kind), this);
    QFont titleFont = roleLabel_->font();
    titleFont.setBold(true);
    roleLabel_->setFont(titleFont);

    if (kind_ == ChatMessageKind::User) {
        roleLabel_->hide();
        contentBrowser_ = new QTextBrowser(this);
        contentBrowser_->setFrameShape(QFrame::NoFrame);
        contentBrowser_->setOpenExternalLinks(true);
        contentBrowser_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        contentBrowser_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        contentBrowser_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        contentBrowser_->document()->setDocumentMargin(14);
        layout->addWidget(contentBrowser_);
        connect(
            contentBrowser_->document()->documentLayout(),
            &QAbstractTextDocumentLayout::documentSizeChanged,
            this,
            [this](const QSizeF& size) {
                contentBrowser_->setFixedHeight(qMax(24, qCeil(size.height()) + 2));
            }
        );
    } else {
        roleLabel_->setContentsMargins(14, 0, 14, 0);
        layout->addWidget(roleLabel_);
        contentLabel_ = new QLabel(this);
        contentLabel_->setContentsMargins(14, 2, 14, 2);
        contentLabel_->setWordWrap(true);
        contentLabel_->setTextInteractionFlags(Qt::TextBrowserInteraction);
        contentLabel_->setOpenExternalLinks(true);
        QSizePolicy contentPolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
        contentPolicy.setHeightForWidth(true);
        contentLabel_->setSizePolicy(contentPolicy);
        layout->addWidget(contentLabel_);
    }
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
    if (kind_ == ChatMessageKind::User) {
        QString body = content_.toHtmlEscaped();
        body.replace(QLatin1Char('\n'), QStringLiteral("<br>"));
        contentBrowser_->setHtml(
            QStringLiteral("<b>%1</b><br>%2").arg(titleFor(kind_), body)
        );
        updateDocumentWidth();
    } else if (kind_ == ChatMessageKind::Assistant) {
        contentLabel_->setTextFormat(Qt::MarkdownText);
        contentLabel_->setText(content_);
    } else {
        contentLabel_->setTextFormat(Qt::PlainText);
        contentLabel_->setText(content_);
    }
}

void ChatMessageWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    if (contentBrowser_) updateDocumentWidth();
}

void ChatMessageWidget::updateDocumentWidth() {
    const int width = contentBrowser_->viewport()->width();
    if (width > 0) contentBrowser_->document()->setTextWidth(width);
}
