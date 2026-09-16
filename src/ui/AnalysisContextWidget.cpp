#include <ui/AnalysisContextWidget.hpp>

#include <QLabel>
#include <QVBoxLayout>

AnalysisContextWidget::AnalysisContextWidget(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    binaryLabel_ = new QLabel(this);
    functionLabel_ = new QLabel(this);
    binaryLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    functionLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(binaryLabel_);
    layout->addWidget(functionLabel_);
    clearAnalysisContext();
}

void AnalysisContextWidget::setAnalysisContext(
    const QString& binaryName,
    const QString& functionName,
    quint64 address
) {
    binaryLabel_->setText(tr("Binary: %1").arg(binaryName));
    functionLabel_->setText(
        tr("Function: %1 at 0x%2").arg(functionName, QString::number(address, 16))
    );
}

void AnalysisContextWidget::clearAnalysisContext() {
    binaryLabel_->setText(tr("Binary: No active analysis context"));
    functionLabel_->clear();
}
