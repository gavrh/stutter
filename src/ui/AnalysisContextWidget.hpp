#pragma once

#include <QWidget>

class QLabel;

class AnalysisContextWidget final : public QWidget {
    Q_OBJECT

public:
    explicit AnalysisContextWidget(QWidget* parent = nullptr);

    void setAnalysisContext(
        const QString& binaryName,
        const QString& functionName,
        quint64 address
    );
    void clearAnalysisContext();

private:
    QLabel* binaryLabel_;
    QLabel* functionLabel_;
};
