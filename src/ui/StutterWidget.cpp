#include <ui/StutterWidget.hpp>
#include <constants.h>

#include <QAbstractTextDocumentLayout>
#include <QComboBox>
#include <QKeyEvent>
#include <QString>
#include <QTextBrowser>
#include <QTextDocument>
#include <QTextEdit>
#include <QLabel>
#include <QtMath>

Window::Window(MainWindow* main) : CutterDockWidget(main) {
    this->setObjectName(STUTTER_DISPLAY_NAME);
    this->setWindowTitle(STUTTER_DISPLAY_NAME.data());
    this->content = new QWidget(this);
    this->setWidget(this->content);
    this->setup();
}

void Window::setup() {
    this->layout = new QBoxLayout(QBoxLayout::Direction::TopToBottom, this->content);
    this->layout->addLayout(this->headerLayout());
    this->layout->addLayout(this->responseLayout(), 1);
    this->layout->addLayout(this->selectionLayout());
    this->layout->addLayout(this->inputLayout());
    this->layout->addLayout(this->footerLayout());
}

QBoxLayout* Window::headerLayout() {
    QBoxLayout* layout = new QBoxLayout(QBoxLayout::Direction::LeftToRight);
    return layout;
}

QBoxLayout* Window::responseLayout() {
    QBoxLayout* layout = new QBoxLayout(QBoxLayout::Direction::LeftToRight);
    auto* view = new QTextBrowser(this->content);
    QString response = QStringLiteral("# Testing");

    view->setMarkdown(response);
    layout->addWidget(view);
    return layout;
}

QBoxLayout* Window::selectionLayout() {
    QBoxLayout* layout = new QBoxLayout(QBoxLayout::Direction::LeftToRight);

    // QComboBox* modelBox = new QComboBox(this->content);
    // for (const auto &model : models) {
    //     modelBox->addItem(QString(model.name.data()));
    // }

    QComboBox* effortBox = new QComboBox(this->content);
    effortBox->addItem(QStringLiteral("Max"));
    effortBox->addItem(QStringLiteral("XHigh"));
    effortBox->addItem(QStringLiteral("High"));
    effortBox->addItem(QStringLiteral("Medium"));
    effortBox->addItem(QStringLiteral("Low"));

    QComboBox* modeBox = new QComboBox(this->content);
    modeBox->addItem(QStringLiteral("Edit"));
    modeBox->addItem(QStringLiteral("Plan"));
    modeBox->addItem(QStringLiteral("Teach"));

    // layout->addWidget(modelBox);
    layout->addWidget(effortBox);
    layout->addWidget(modeBox);
    return layout;
}

QBoxLayout* Window::inputLayout() {
    QBoxLayout* layout = new QBoxLayout(QBoxLayout::Direction::LeftToRight);
    QTextEdit* input = new CustomTextEdit(this->content);
    input->setPlaceholderText(QStringLiteral("You can do reverse engineering, but you can’t do reverse hacking"));

    layout->addWidget(input);
    return layout;
}

QBoxLayout* Window::footerLayout() {
    QBoxLayout* layout = new QBoxLayout(QBoxLayout::Direction::LeftToRight);
    QLabel* usage = new QLabel(QString("120k (25%)"), this->content);
    QLabel* version = new QLabel(QString(STUTTER_VERSION_STR.data()), this->content);

    layout->addWidget(usage, 1, Qt::AlignLeft);
    layout->addWidget(version, 0, Qt::AlignRight);
    return layout;
}

CustomTextEdit::CustomTextEdit(QWidget* parent) : QTextEdit(parent) {
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    this->setLineWrapMode(QTextEdit::WidgetWidth);
    this->setViewportMargins(0, verticalPadding, 0, verticalPadding);

    connect(
        this->document()->documentLayout(),
        &QAbstractTextDocumentLayout::documentSizeChanged,
        this,
        [this](const QSizeF&) {
            this->updateGeometry();
        }
    );
}

QSize CustomTextEdit::sizeHint() const {
    QSize hint = QTextEdit::sizeHint();
    hint.setHeight(qBound(
        this->minimumContentHeight(),
        this->documentHeight(),
        maximumContentHeight
    ));
    return hint;
}

QSize CustomTextEdit::minimumSizeHint() const {
    return QSize(QTextEdit::minimumSizeHint().width(), this->minimumContentHeight());
}

void CustomTextEdit::keyPressEvent(QKeyEvent* event) {
    const bool enter = event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter;

    if (enter && !(event->modifiers() & Qt::ShiftModifier)) {
        const QString text = this->toPlainText();
        if (!text.trimmed().isEmpty()) {
            emit submitted(text);
            this->clear();
        }

        event->accept();
        return;
    }

    QTextEdit::keyPressEvent(event);
}

int CustomTextEdit::chromeHeight() const {
    const QMargins margins = this->viewportMargins();
    return 2 * this->frameWidth() + margins.top() + margins.bottom();
}

int CustomTextEdit::minimumContentHeight() const {
    return this->fontMetrics().lineSpacing()
        + qCeil(2 * this->document()->documentMargin())
        + this->chromeHeight();
}

int CustomTextEdit::documentHeight() const {
    return qCeil(this->document()->documentLayout()->documentSize().height())
        + this->chromeHeight();
}
