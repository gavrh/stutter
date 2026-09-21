#pragma once

#include <providers/ProviderTypes.hpp>
#include <tools/ToolPermission.hpp>
#include <tools/ToolResult.hpp>

#include <QJsonArray>
#include <QJsonObject>
#include <QString>

#include <functional>

namespace stutter {

using ToolHandler = std::function<ToolResult(const QJsonObject& arguments)>;

struct Tool {
    QString name;
    QString description;
    QJsonObject inputSchema;
    ToolPermission permission = ToolPermission::Read;
    ToolHandler handler;

    ToolDefinition definition() const {
        return ToolDefinition {name, description, inputSchema};
    }
};

inline QJsonObject stringProperty(const QString& description) {
    return QJsonObject {
        {QStringLiteral("type"), QStringLiteral("string")},
        {QStringLiteral("description"), description}
    };
}

inline QJsonObject integerProperty(const QString& description, int minimum, int maximum) {
    return QJsonObject {
        {QStringLiteral("type"), QStringLiteral("integer")},
        {QStringLiteral("description"), description},
        {QStringLiteral("minimum"), minimum},
        {QStringLiteral("maximum"), maximum}
    };
}

inline QJsonObject toolSchema(const QJsonObject& properties, const QJsonArray& required) {
    return QJsonObject {
        {QStringLiteral("type"), QStringLiteral("object")},
        {QStringLiteral("properties"), properties},
        {QStringLiteral("required"), required}
    };
}

inline Tool makeTool(
    const QString& name,
    const QString& description,
    const QJsonObject& schema,
    ToolPermission permission,
    ToolHandler handler
) {
    Tool tool;
    tool.name = name;
    tool.description = description;
    tool.inputSchema = schema;
    tool.permission = permission;
    tool.handler = std::move(handler);
    return tool;
}

}
