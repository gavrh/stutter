#include <ui/SettingsDialog.hpp>

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("Stutter Settings"));
    setModal(false);
    resize(480, 360);

    auto* root = new QVBoxLayout(this);
    auto* providerForm = new QFormLayout;
    providerBox_ = new QComboBox(this);
    providerBox_->addItem(QStringLiteral("OpenAI"), QStringLiteral("openai"));
    providerBox_->addItem(QStringLiteral("Anthropic"), QStringLiteral("anthropic"));
    modelEdit_ = new QLineEdit(this);
    modelEdit_->setPlaceholderText(QStringLiteral("gpt-4o or claude-sonnet-4-20250514"));
    apiKeyEdit_ = new QLineEdit(this);
    apiKeyEdit_->setEchoMode(QLineEdit::Password);
    apiKeyEdit_->setPlaceholderText(tr("API key"));
    endpointEdit_ = new QLineEdit(this);
    endpointEdit_->setPlaceholderText(tr("Optional custom API base URL"));
    providerForm->addRow(tr("Provider"), providerBox_);
    providerForm->addRow(tr("Model"), modelEdit_);
    providerForm->addRow(tr("API key"), apiKeyEdit_);
    providerForm->addRow(tr("Endpoint"), endpointEdit_);

    auto* permissionBox = new QGroupBox(tr("Tool permissions"), this);
    auto* permissionLayout = new QVBoxLayout(permissionBox);
    permissionLayout->addWidget(new QLabel(
        tr("Read-only inspection does not require confirmation."), permissionBox
    ));
    analysisPermission_ = new QCheckBox(tr("Allow analysis changes"), permissionBox);
    binaryPermission_ = new QCheckBox(tr("Allow binary modifications"), permissionBox);
    debuggerPermission_ = new QCheckBox(tr("Allow debugger control"), permissionBox);
    permissionLayout->addWidget(analysisPermission_);
    permissionLayout->addWidget(binaryPermission_);
    permissionLayout->addWidget(debuggerPermission_);

    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel,
        this
    );
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    root->addLayout(providerForm);
    root->addWidget(permissionBox);
    root->addStretch();
    root->addWidget(buttons);
}

QString SettingsDialog::providerId() const {
    return providerBox_->currentData().toString();
}

void SettingsDialog::setProviderId(const QString& providerId) {
    const int index = providerBox_->findData(providerId);
    if (index >= 0) providerBox_->setCurrentIndex(index);
}

QString SettingsDialog::model() const { return modelEdit_->text().trimmed(); }
void SettingsDialog::setModel(const QString& model) { modelEdit_->setText(model); }
QString SettingsDialog::apiKey() const { return apiKeyEdit_->text(); }
void SettingsDialog::setApiKey(const QString& apiKey) { apiKeyEdit_->setText(apiKey); }
QString SettingsDialog::endpoint() const { return endpointEdit_->text().trimmed(); }
void SettingsDialog::setEndpoint(const QString& endpoint) { endpointEdit_->setText(endpoint); }
bool SettingsDialog::allowAnalysisChanges() const { return analysisPermission_->isChecked(); }
bool SettingsDialog::allowBinaryChanges() const { return binaryPermission_->isChecked(); }
bool SettingsDialog::allowDebuggerControl() const { return debuggerPermission_->isChecked(); }
