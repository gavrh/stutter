#include <ui/SettingsDialog.hpp>

#include <config/ModelCatalog.hpp>
#include <providers/CodexProvider.hpp>

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

namespace {
QString effortName(const QString& effort) {
    if (effort == QStringLiteral("none")) return SettingsDialog::tr("None");
    if (effort == QStringLiteral("low")) return SettingsDialog::tr("Low");
    if (effort == QStringLiteral("medium")) return SettingsDialog::tr("Medium");
    if (effort == QStringLiteral("high")) return SettingsDialog::tr("High");
    if (effort == QStringLiteral("xhigh")) return SettingsDialog::tr("Extra High");
    if (effort == QStringLiteral("max")) return SettingsDialog::tr("Maximum");
    if (effort == QStringLiteral("ultra")) return SettingsDialog::tr("Ultra");
    return effort;
}

QString nearestLowerEffort(const QString& effort, const QStringList& supported) {
    const QStringList order {
        QStringLiteral("none"),
        QStringLiteral("low"),
        QStringLiteral("medium"),
        QStringLiteral("high"),
        QStringLiteral("xhigh"),
        QStringLiteral("max"),
        QStringLiteral("ultra")
    };
    for (int index = order.indexOf(effort) - 1; index >= 0; --index) {
        if (supported.contains(order.at(index))) return order.at(index);
    }
    return supported.isEmpty() ? QString() : supported.first();
}
}

SettingsDialog::SettingsDialog(
    const ModelCatalog& modelCatalog,
    CodexProvider& codexProvider,
    QWidget* parent
) : QDialog(parent), modelCatalog_(modelCatalog), codexProvider_(codexProvider) {
    setWindowTitle(tr("Stutter Settings"));
    setModal(false);
    resize(480, 360);

    auto* root = new QVBoxLayout(this);
    auto* providerForm = new QFormLayout;
    providerBox_ = new QComboBox(this);
    providerBox_->addItem(QStringLiteral("OpenAI"), QStringLiteral("openai"));
    providerBox_->addItem(QStringLiteral("Anthropic"), QStringLiteral("anthropic"));
    providerBox_->addItem(QStringLiteral("Codex (ChatGPT)"), QStringLiteral("codex"));
    modelBox_ = new QComboBox(this);
    effortBox_ = new QComboBox(this);
    apiKeyEdit_ = new QLineEdit(this);
    apiKeyEdit_->setEchoMode(QLineEdit::Password);
    apiKeyEdit_->setPlaceholderText(tr("API key"));
    endpointEdit_ = new QLineEdit(this);
    endpointEdit_->setPlaceholderText(tr("Optional custom API base URL"));
    apiKeyLabel_ = new QLabel(tr("API key"), this);
    endpointLabel_ = new QLabel(tr("Endpoint"), this);
    providerForm->addRow(tr("Provider"), providerBox_);
    providerForm->addRow(tr("Model"), modelBox_);
    providerForm->addRow(tr("Effort"), effortBox_);
    providerForm->addRow(apiKeyLabel_, apiKeyEdit_);
    providerForm->addRow(endpointLabel_, endpointEdit_);

    codexBox_ = new QGroupBox(tr("Codex"), this);
    auto* codexLayout = new QVBoxLayout(codexBox_);
    codexStatus_ = new QLabel(codexProvider_.connectionStatus(), codexBox_);
    codexStatus_->setWordWrap(true);
    codexLayout->addWidget(codexStatus_);

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
    root->addWidget(codexBox_);
    root->addWidget(permissionBox);
    root->addStretch();
    root->addWidget(buttons);

    connect(providerBox_, &QComboBox::currentIndexChanged, this, [this] {
        updateModels();
        endpointEdit_->setText(modelCatalog_.defaultEndpoint(providerId()).toString());
        updateProviderUi();
    });
    connect(modelBox_, &QComboBox::currentIndexChanged, this, &SettingsDialog::updateEfforts);
    connect(effortBox_, &QComboBox::activated, this, [this](int) {
        if (!effort().isEmpty()) preferredEffort_ = effort();
    });
    connect(&codexProvider_, &CodexProvider::connectionStatusChanged, this,
        [this](const QString& text, bool, bool) {
            codexStatus_->setText(text);
        });
    updateModels();
    endpointEdit_->setText(modelCatalog_.defaultEndpoint(providerId()).toString());
    updateProviderUi();
}

QString SettingsDialog::providerId() const {
    return providerBox_->currentData().toString();
}

void SettingsDialog::setProviderId(const QString& providerId) {
    const int index = providerBox_->findData(providerId);
    if (index >= 0) providerBox_->setCurrentIndex(index);
}

QString SettingsDialog::model() const { return modelBox_->currentData().toString(); }

void SettingsDialog::setModel(const QString& model) {
    int index = modelBox_->findData(model);
    if (index >= 0) {
        modelBox_->setCurrentIndex(index);
        return;
    }

    for (int providerIndex = 0; providerIndex < providerBox_->count(); ++providerIndex) {
        const QString candidateProvider = providerBox_->itemData(providerIndex).toString();
        for (const stutter::Model& candidate : modelCatalog_.modelsForProvider(candidateProvider)) {
            if (candidate.id == model) {
                providerBox_->setCurrentIndex(providerIndex);
                index = modelBox_->findData(model);
                if (index >= 0) modelBox_->setCurrentIndex(index);
                return;
            }
        }
    }
}
QString SettingsDialog::effort() const { return effortBox_->currentData().toString(); }

void SettingsDialog::setEffort(const QString& effort) {
    preferredEffort_ = effort;
    const int index = effortBox_->findData(effort);
    if (index >= 0) effortBox_->setCurrentIndex(index);
}
QString SettingsDialog::apiKey() const { return apiKeyEdit_->text(); }
void SettingsDialog::setApiKey(const QString& apiKey) { apiKeyEdit_->setText(apiKey); }
QString SettingsDialog::endpoint() const { return endpointEdit_->text().trimmed(); }
void SettingsDialog::setEndpoint(const QString& endpoint) { endpointEdit_->setText(endpoint); }
bool SettingsDialog::allowAnalysisChanges() const { return analysisPermission_->isChecked(); }
bool SettingsDialog::allowBinaryChanges() const { return binaryPermission_->isChecked(); }
bool SettingsDialog::allowDebuggerControl() const { return debuggerPermission_->isChecked(); }

void SettingsDialog::updateModels() {
    const QString selectedModel = model();
    modelBox_->clear();
    for (const stutter::Model& model : modelCatalog_.modelsForProvider(providerId())) {
        modelBox_->addItem(model.name, model.id);
    }

    int index = modelBox_->findData(selectedModel);
    if (index < 0) {
        index = modelBox_->findData(modelCatalog_.defaultModel(providerId()));
    }
    if (index >= 0) modelBox_->setCurrentIndex(index);
    modelBox_->setEnabled(modelCatalog_.isValid() && modelBox_->count() > 0);
    modelBox_->setToolTip(modelCatalog_.isValid() ? QString() : modelCatalog_.error());
    updateEfforts();
}

void SettingsDialog::updateEfforts() {
    effortBox_->clear();

    stutter::Model selectedModel;
    for (const stutter::Model& candidate : modelCatalog_.modelsForProvider(providerId())) {
        if (candidate.id == model()) {
            selectedModel = candidate;
            break;
        }
    }
    if (selectedModel.supportedEfforts.isEmpty()) {
        effortBox_->addItem(tr("Default"), QString());
        effortBox_->setEnabled(false);
        return;
    }
    for (const QString& supportedEffort : selectedModel.supportedEfforts) {
        effortBox_->addItem(effortName(supportedEffort), supportedEffort);
    }
    if (preferredEffort_.isEmpty()) preferredEffort_ = selectedModel.defaultEffort;
    int index = effortBox_->findData(preferredEffort_);
    if (index < 0) {
        preferredEffort_ = nearestLowerEffort(
            preferredEffort_,
            selectedModel.supportedEfforts
        );
        index = effortBox_->findData(preferredEffort_);
    }
    if (index >= 0) effortBox_->setCurrentIndex(index);
    effortBox_->setEnabled(true);
}

void SettingsDialog::updateProviderUi() {
    const bool codexSelected = providerId() == QStringLiteral("codex");
    codexBox_->setVisible(codexSelected);
    apiKeyLabel_->setVisible(!codexSelected);
    apiKeyEdit_->setVisible(!codexSelected);
    endpointLabel_->setVisible(!codexSelected);
    endpointEdit_->setVisible(!codexSelected);
    if (codexSelected && !codexProvider_.isConnected()) {
        codexProvider_.connectChatGpt();
    } else if (!codexSelected) {
        codexProvider_.disconnect();
    }
}
