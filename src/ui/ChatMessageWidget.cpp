#include <ui/ChatMessageWidget.hpp>

#include <tools/ToolProtocol.hpp>

#include <QAbstractTextDocumentLayout>
#include <QFrame>
#include <QLabel>
#include <QResizeEvent>
#include <QTextBrowser>
#include <QTextDocument>
#include <QTextOption>
#include <QVBoxLayout>
#include <QtMath>

namespace {
QString wrapLongRuns(const QString& text, int maxRun = 32) {
    QString result;
    result.reserve(text.size() + text.size() / maxRun);
    int run = 0;
    bool inLinkTarget = false;
    for (int index = 0; index < text.size(); ++index) {
        const QChar character = text.at(index);
        if (!inLinkTarget && character == QLatin1Char('(')
            && index > 0 && text.at(index - 1) == QLatin1Char(']')) {
            inLinkTarget = true;
        } else if (inLinkTarget && character == QLatin1Char(')')) {
            inLinkTarget = false;
        }
        result.append(character);
        if (inLinkTarget) continue;
        if (character.isSpace()) {
            run = 0;
            continue;
        }
        ++run;
        if (run >= maxRun) {
            result.append(QChar(0x200B));
            run = 0;
        }
    }
    return result;
}

QString titleFor(ChatMessageKind kind) {
    switch (kind) {
    case ChatMessageKind::User: return QStringLiteral("You");
    case ChatMessageKind::Assistant: return QStringLiteral("Stutter");
    case ChatMessageKind::Error: return QStringLiteral("Error");
    case ChatMessageKind::Tool: return QStringLiteral("Tool");
    }
    return {};
}

QString escapedWrapped(const QString& text) {
    QString value = wrapLongRuns(text.toHtmlEscaped());
    value.replace(QLatin1Char('\n'), QStringLiteral("<br>"));
    return value;
}

QString activityHtml(const stutter::ToolActivity& activity) {
    QString html = QStringLiteral("<b>%1</b>").arg(activity.name.toHtmlEscaped());
    if (!activity.detail.isEmpty()) {
        html += QStringLiteral("<br>%1").arg(escapedWrapped(activity.detail));
    }
    if (!activity.result.isEmpty()) {
        html += QStringLiteral("<br>%1").arg(escapedWrapped(activity.result));
    }
    return html;
}
}

class ActivityBlock final : public QWidget {
public:
    explicit ActivityBlock(QWidget* parent = nullptr) : QWidget(parent) {
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(14, 2, 14, 2);
        layout->setSpacing(0);

        label_ = new QLabel(this);
        label_->setWordWrap(true);
        label_->setTextFormat(Qt::RichText);
        label_->setTextInteractionFlags(Qt::TextBrowserInteraction);
        label_->setOpenExternalLinks(true);
        QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Minimum);
        policy.setHeightForWidth(true);
        label_->setSizePolicy(policy);
        layout->addWidget(label_);
    }

    void setContent(const QString& html) {
        label_->setText(html);
    }

    void updateHeight(int width) {
        const int inner = width - 28;
        if (inner <= 0) return;
        QTextDocument document;
        document.setDefaultFont(label_->font());
        document.setDocumentMargin(0);
        document.setTextWidth(inner);
        document.setHtml(label_->text());
        label_->setFixedHeight(qCeil(document.size().height()));
    }

private:
    QLabel* label_;
};

ChatMessageWidget::ChatMessageWidget(
    ChatMessageKind kind,
    const QString& content,
    QWidget* parent
) : QWidget(parent), kind_(kind), content_(content), segmentText_(content) {
    setObjectName(QStringLiteral("chatMessage"));
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
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
        contentBrowser_->setLineWrapMode(QTextEdit::WidgetWidth);
        contentBrowser_->setWordWrapMode(QTextOption::WrapAnywhere);
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
        if (kind_ == ChatMessageKind::Assistant) {
            bodyLayout_ = new QVBoxLayout;
            bodyLayout_->setContentsMargins(0, 0, 0, 0);
            bodyLayout_->setSpacing(6);
            layout->addLayout(bodyLayout_);
        } else {
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
    }
    render();
}

void ChatMessageWidget::setTitle(const QString& title) {
    if (roleLabel_) roleLabel_->setText(title);
}

void ChatMessageWidget::setContent(const QString& content) {
    content_ = content;
    render();
}

void ChatMessageWidget::appendContent(const QString& content) {
    content_.append(content);
    if (kind_ != ChatMessageKind::Assistant) {
        render();
        return;
    }
    if (!activeTextLabel_) {
        segmentText_ = content;
        addTextLabel(segmentText_);
    } else {
        segmentText_.append(content);
        activeTextLabel_->setText(wrapLongRuns(stutter::stripToolBlocks(segmentText_)));
        updateTextLabelHeight(activeTextLabel_);
    }
}

void ChatMessageWidget::setActivity(const stutter::ToolActivity& activity) {
    if (!bodyLayout_) return;

    ActivityBlock* block = activityBlocks_.value(activity.id);
    if (!block) {
        activeTextLabel_ = nullptr;
        segmentText_.clear();
        block = createActivityBlock();
        bodyLayout_->addWidget(block);
        activityBlocks_.insert(activity.id, block);
    }
    block->setContent(activityHtml(activity));
    block->setVisible(true);
    block->updateHeight(this->width());
}

void ChatMessageWidget::render() {
    if (kind_ == ChatMessageKind::User) {
        QString body = content_.toHtmlEscaped();
        body.replace(QLatin1Char('\n'), QStringLiteral("<br>"));
        contentBrowser_->setHtml(
            QStringLiteral("<b>%1</b><br>%2").arg(titleFor(kind_), body)
        );
        updateBrowserWidth(contentBrowser_);
    } else if (kind_ == ChatMessageKind::Assistant) {
        if (!content_.isEmpty() && textLabels_.isEmpty()) {
            addTextLabel(content_);
        }
    } else {
        contentLabel_->setTextFormat(Qt::PlainText);
        contentLabel_->setText(content_);
        updateTextLabelHeight(contentLabel_);
    }
}

void ChatMessageWidget::addTextLabel(const QString& text) {
    auto* label = new QLabel(this);
    label->setContentsMargins(14, 2, 14, 2);
    label->setWordWrap(true);
    label->setTextFormat(Qt::MarkdownText);
    label->setTextInteractionFlags(Qt::TextBrowserInteraction);
    label->setOpenExternalLinks(true);
    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    policy.setHeightForWidth(true);
    label->setSizePolicy(policy);
    label->setText(wrapLongRuns(stutter::stripToolBlocks(text)));
    bodyLayout_->addWidget(label);
    textLabels_.append(label);
    activeTextLabel_ = label;
    updateTextLabelHeight(label);
}

ActivityBlock* ChatMessageWidget::createActivityBlock() {
    return new ActivityBlock(this);
}

void ChatMessageWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    if (contentBrowser_) updateBrowserWidth(contentBrowser_);
    for (ActivityBlock* block : activityBlocks_) {
        block->updateHeight(this->width());
    }
    for (QLabel* label : textLabels_) {
        updateTextLabelHeight(label);
    }
    if (contentLabel_) updateTextLabelHeight(contentLabel_);
}

void ChatMessageWidget::updateBrowserWidth(QTextBrowser* browser) {
    const int width = browser->viewport()->width();
    if (width > 0) browser->document()->setTextWidth(width);
}

void ChatMessageWidget::updateTextLabelHeight(QLabel* label) {
    if (!label) return;
    const int width = this->width();
    if (width <= 0) return;

    const QMargins margins = label->contentsMargins();
    const int textWidth = width - margins.left() - margins.right();
    if (textWidth <= 0) return;

    QTextDocument document;
    document.setDefaultFont(label->font());
    document.setDocumentMargin(0);
    document.setTextWidth(textWidth);
    if (label->textFormat() == Qt::MarkdownText) {
        document.setMarkdown(label->text());
    } else {
        document.setPlainText(label->text());
    }

    const int height = qCeil(document.size().height())
        + margins.top() + margins.bottom();
    label->setFixedHeight(height);
}
