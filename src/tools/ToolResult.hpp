#pragma once

#include <QString>

namespace stutter {

struct ToolResult {
    bool success = true;
    QString content;
    QString error;

    static ToolResult ok(const QString& content) {
        ToolResult result;
        result.success = true;
        result.content = content;
        return result;
    }

    static ToolResult fail(const QString& error) {
        ToolResult result;
        result.success = false;
        result.error = error;
        return result;
    }
};

}
