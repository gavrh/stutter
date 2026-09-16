#pragma once

#include <QDialog>

class ModelCatalog;
class CodexProvider;
class QCheckBox;
class QComboBox;
class QLineEdit;
class QLabel;
class QGroupBox;

class SettingsDialog final : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(
        const ModelCatalog& modelCatalog,
        CodexProvider& codexProvider,
        QWidget* parent = nullptr
    );

    QString providerId() const;
    void setProviderId(const QString& providerId);
    QString model() const;
    void setModel(const QString& model);
    QString effort() const;
    void setEffort(const QString& effort);
    QString apiKey() const;
    void setApiKey(const QString& apiKey);
    QString endpoint() const;
    void setEndpoint(const QString& endpoint);

    bool allowAnalysisChanges() const;
    bool allowBinaryChanges() const;
    bool allowDebuggerControl() const;

private:
    void updateModels();
    void updateEfforts();
    void updateProviderUi();

    const ModelCatalog& modelCatalog_;
    CodexProvider& codexProvider_;
    QComboBox* providerBox_;
    QComboBox* modelBox_;
    QComboBox* effortBox_;
    QLineEdit* apiKeyEdit_;
    QLineEdit* endpointEdit_;
    QLabel* apiKeyLabel_;
    QLabel* endpointLabel_;
    QGroupBox* codexBox_;
    QLabel* codexStatus_;
    QCheckBox* analysisPermission_;
    QCheckBox* binaryPermission_;
    QCheckBox* debuggerPermission_;
    QString preferredEffort_;
};
