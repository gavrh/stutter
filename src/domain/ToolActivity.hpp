#pragma once

#include <QDateTime>
#include <QMetaType>
#include <QString>

namespace stutter {

enum class ToolActivityStatus {
    Running,
    Succeeded,
    Failed
};

enum class ToolActivityCategory {
    Provider,
    Reader,
    Analysis,
    Binary,
    Debugger
};

struct ToolActivity {
    QString id;
    QString name;
    ToolActivityCategory category = ToolActivityCategory::Provider;
    ToolActivityStatus status = ToolActivityStatus::Running;
    QString detail;
    QString result;
    QDateTime startedAt;
    QDateTime finishedAt;
};

}

Q_DECLARE_METATYPE(stutter::ToolActivity)
