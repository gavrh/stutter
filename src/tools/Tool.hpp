#pragma once

#include <providers/ProviderTypes.hpp>
#include <tools/ToolPermission.hpp>
#include <tools/ToolResult.hpp>

#include <core/CutterCommon.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVector>

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

inline QJsonObject addressProperty(const QString& description) {
    return QJsonObject {
        {QStringLiteral("type"), QJsonArray {QStringLiteral("string"), QStringLiteral("integer")}},
        {QStringLiteral("description"), description}
    };
}

inline QJsonObject addressesProperty(const QString& description) {
    return QJsonObject {
        {QStringLiteral("type"), QJsonArray {
            QStringLiteral("array"), QStringLiteral("string"), QStringLiteral("integer")
        }},
        {QStringLiteral("items"), QJsonObject {
            {QStringLiteral("type"), QJsonArray {QStringLiteral("string"), QStringLiteral("integer")}}
        }},
        {QStringLiteral("description"), description}
    };
}

inline QString combineResults(
    const QVector<RVA>& addresses,
    const std::function<QString(RVA)>& render
) {
    if (addresses.size() == 1) return render(addresses.first());
    QStringList parts;
    for (RVA address : addresses) {
        parts.append(QStringLiteral("### 0x%1").arg(address, 0, 16));
        parts.append(render(address));
    }
    return parts.join(QLatin1Char('\n'));
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
