#include <tools/ToolExecutor.hpp>

#include <utility>

namespace stutter {

ToolExecutor::ToolExecutor(ToolRegistry& registry, ToolPermissionCheck permissionCheck)
    : registry_(registry), permissionCheck_(std::move(permissionCheck)) {}

bool ToolExecutor::isKnown(const QString& name) const {
    return registry_.contains(name);
}

bool ToolExecutor::canUse(const QString& name) const {
    const Tool* tool = registry_.find(name);
    if (!tool) return false;
    if (!permissionCheck_) return true;
    return permissionCheck_(tool->permission);
}

ToolPermission ToolExecutor::permissionFor(const QString& name) const {
    const Tool* tool = registry_.find(name);
    return tool ? tool->permission : ToolPermission::Read;
}

ToolResult ToolExecutor::execute(const ToolCall& call) {
    const Tool* tool = registry_.find(call.name);
    if (!tool) {
        return ToolResult::fail(QStringLiteral("Unknown tool: %1").arg(call.name));
    }
    if (permissionCheck_ && !permissionCheck_(tool->permission)) {
        return ToolResult::fail(
            QStringLiteral("Permission denied for tool: %1").arg(call.name)
        );
    }
    if (!tool->handler) {
        return ToolResult::fail(QStringLiteral("Tool has no handler: %1").arg(call.name));
    }

    try {
        return tool->handler(call.arguments);
    } catch (const std::exception& exception) {
        return ToolResult::fail(QString::fromUtf8(exception.what()));
    } catch (...) {
        return ToolResult::fail(QStringLiteral("Tool failed: %1").arg(call.name));
    }
}

}
