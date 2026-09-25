#include <cutter/CutterGateway.hpp>
#include <tools/ToolRegistry.hpp>
#include <tools/ToolValidator.hpp>

#include <QJsonArray>
#include <QJsonObject>
#include <QStringList>

#include <functional>

static QJsonObject addressesProperty(const QString& description) {
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

static QString combineResults(const QVector<RVA>& addresses, const std::function<QString(RVA)>& render) {
    if (addresses.size() == 1) return render(addresses.first());
    QStringList parts;
    for (RVA address : addresses) {
        parts.append(QStringLiteral("### 0x%1").arg(address, 0, 16));
        parts.append(render(address));
    }
    return parts.join(QLatin1Char('\n'));
}

namespace stutter {

void registerAnalysisTools(ToolRegistry& registry, CutterGateway& gateway) {
    RizinReader& reader = gateway.reader();

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
