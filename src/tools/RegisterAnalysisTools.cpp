#include <cutter/CutterGateway.hpp>
#include <tools/ToolRegistry.hpp>
#include <tools/ToolValidator.hpp>

#include <QJsonArray>
#include <QJsonObject>
#include <QStringList>

#include <functional>

namespace {
QJsonObject addressesProperty(const QString& description) {
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

QString combineResults(const QVector<RVA>& addresses, const std::function<QString(RVA)>& render) {
    if (addresses.size() == 1) return render(addresses.first());
    QStringList parts;
    for (RVA address : addresses) {
        parts.append(QStringLiteral("### 0x%1").arg(address, 0, 16));
        parts.append(render(address));
    }
    return parts.join(QLatin1Char('\n'));
}

QString functionDescription(RizinReader& reader, RVA address) {
    const RVA start = reader.functionStart(address);
    if (start == RVA_INVALID) {
        return QStringLiteral("No function at 0x%1.").arg(address, 0, 16);
    }
    return QStringLiteral("name: %1\nstart: 0x%2\nend: 0x%3")
        .arg(reader.functionName(address))
        .arg(start, 0, 16)
        .arg(reader.functionEnd(address), 0, 16);
}
}

namespace stutter {

void registerAnalysisTools(ToolRegistry& registry, CutterGateway& gateway) {
    RizinReader& reader = gateway.reader();

    registry.add(makeTool(
        QStringLiteral("current_function"),
        QStringLiteral("Describe the function at the current address."),
        toolSchema({}, {}),
        ToolPermission::Read,
        [&reader](const QJsonObject&) {
            return ToolResult::ok(functionDescription(reader, reader.currentAddress()));
        }
    ));

    registry.add(makeTool(
        QStringLiteral("function_info"),
        QStringLiteral("Describe the function containing one or more addresses."),
        toolSchema(
            {{QStringLiteral("addresses"), addressesProperty(QStringLiteral("Address or list of addresses"))}},
            {QStringLiteral("addresses")}
        ),
        ToolPermission::Read,
        [&reader](const QJsonObject& arguments) {
            QVector<RVA> targets;
            QString error;
            if (!ToolValidator::addresses(arguments, targets, error)) {
                return ToolResult::fail(error);
            }
            return ToolResult::ok(combineResults(targets, [&reader](RVA address) {
                return functionDescription(reader, address);
            }));
        }
    ));

    registry.add(makeTool(
        QStringLiteral("get_comment"),
        QStringLiteral("Return the comment at one or more addresses."),
        toolSchema(
            {{QStringLiteral("addresses"), addressesProperty(QStringLiteral("Address or list of addresses"))}},
            {QStringLiteral("addresses")}
        ),
        ToolPermission::Read,
        [&reader](const QJsonObject& arguments) {
            QVector<RVA> targets;
            QString error;
            if (!ToolValidator::addresses(arguments, targets, error)) {
                return ToolResult::fail(error);
            }
            return ToolResult::ok(combineResults(targets, [&reader](RVA address) {
                const QString comment = reader.commentAt(address);
                return comment.isEmpty() ? QStringLiteral("No comment.") : comment;
            }));
        }
    ));
}

}
