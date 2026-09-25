#include <cutter/CutterGateway.hpp>
#include <tools/ToolRegistry.hpp>
#include <tools/ToolValidator.hpp>

#include <QJsonArray>
#include <QJsonObject>
#include <QStringList>

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
