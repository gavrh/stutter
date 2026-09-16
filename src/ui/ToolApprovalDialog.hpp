#pragma once

#include <QDialog>

class ToolApprovalDialog final : public QDialog {
    Q_OBJECT

public:
    explicit ToolApprovalDialog(
        const QString& toolName,
        const QString& description,
        const QString& arguments,
        QWidget* parent = nullptr
    );

    static bool requestApproval(
        const QString& toolName,
        const QString& description,
        const QString& arguments,
        QWidget* parent = nullptr
    );
};
