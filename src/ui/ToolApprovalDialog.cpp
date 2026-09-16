#include <ui/ToolApprovalDialog.hpp>

#include <QDialogButtonBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

ToolApprovalDialog::ToolApprovalDialog(
    const QString& toolName,
    const QString& description,
    const QString& arguments,
    QWidget* parent
) : QDialog(parent) {
    setWindowTitle(tr("Approve Tool Action"));
    setModal(true);
    resize(520, 360);

    auto* layout = new QVBoxLayout(this);
    auto* title = new QLabel(tr("Allow %1?").arg(toolName), this);
    QFont titleFont = title->font();
    titleFont.setBold(true);
    title->setFont(titleFont);
    auto* explanation = new QLabel(description, this);
    explanation->setWordWrap(true);
    auto* warning = new QLabel(
        tr("Review the arguments before allowing this action. Approval applies once."),
        this
    );
    warning->setWordWrap(true);
    auto* argumentView = new QPlainTextEdit(arguments, this);
    argumentView->setReadOnly(true);

    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Yes | QDialogButtonBox::No,
        this
    );
    buttons->button(QDialogButtonBox::Yes)->setText(tr("Allow once"));
    buttons->button(QDialogButtonBox::No)->setText(tr("Deny"));
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    layout->addWidget(title);
    layout->addWidget(explanation);
    layout->addWidget(warning);
    layout->addWidget(argumentView, 1);
    layout->addWidget(buttons);
}

bool ToolApprovalDialog::requestApproval(
    const QString& toolName,
    const QString& description,
    const QString& arguments,
    QWidget* parent
) {
    ToolApprovalDialog dialog(toolName, description, arguments, parent);
    return dialog.exec() == QDialog::Accepted;
}
