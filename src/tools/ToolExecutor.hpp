#pragma once

#include <tools/ToolCall.hpp>
#include <tools/ToolPermission.hpp>
#include <tools/ToolRegistry.hpp>
#include <tools/ToolResult.hpp>

#include <QString>

#include <functional>

namespace stutter {

using ToolPermissionCheck = std::function<bool(ToolPermission)>;

class ToolExecutor {
public:
    ToolExecutor(ToolRegistry& registry, ToolPermissionCheck permissionCheck);

    bool isKnown(const QString& name) const;
    bool canUse(const QString& name) const;
    ToolPermission permissionFor(const QString& name) const;
    ToolResult execute(const ToolCall& call);

private:
    ToolRegistry& registry_;
    ToolPermissionCheck permissionCheck_;
};

}
