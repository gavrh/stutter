#pragma once

#include <QDialog>

class QCheckBox;
class QComboBox;
class QLineEdit;

class SettingsDialog final : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr);

    QString providerId() const;
    void setProviderId(const QString& providerId);
    QString model() const;
    void setModel(const QString& model);
    QString apiKey() const;
    void setApiKey(const QString& apiKey);
    QString endpoint() const;
    void setEndpoint(const QString& endpoint);

    bool allowAnalysisChanges() const;
    bool allowBinaryChanges() const;
    bool allowDebuggerControl() const;

private:
    QComboBox* providerBox_;
    QLineEdit* modelEdit_;
    QLineEdit* apiKeyEdit_;
    QLineEdit* endpointEdit_;
    QCheckBox* analysisPermission_;
    QCheckBox* binaryPermission_;
    QCheckBox* debuggerPermission_;
};
