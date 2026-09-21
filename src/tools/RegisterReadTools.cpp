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

QString contextSummary(CutterGateway& gateway) {
    RizinReader& reader = gateway.reader();
    QStringList lines;
    const QString file = reader.fileName();
    if (!file.isEmpty()) lines.append(QStringLiteral("file: %1").arg(file));
    const RVA address = reader.currentAddress();
    lines.append(QStringLiteral("current address: 0x%1").arg(address, 0, 16));
    const RVA start = reader.functionStart(address);
    if (start != RVA_INVALID) {
        lines.append(QStringLiteral("current function: %1 (0x%2-0x%3)")
            .arg(reader.functionName(address))
            .arg(start, 0, 16)
            .arg(reader.functionEnd(address), 0, 16));
    }
    lines.append(QStringLiteral("debugging: %1").arg(
        gateway.debugger().isDebugging() ? QStringLiteral("yes") : QStringLiteral("no")
    ));
    return lines.join(QLatin1Char('\n'));
}
}

namespace stutter {

void registerReadTools(ToolRegistry& registry, CutterGateway& gateway) {
    RizinReader& reader = gateway.reader();

    registry.add(makeTool(
        QStringLiteral("get_context"),
        QStringLiteral("Return the current analysis context: file, address, function, debug state."),
        toolSchema({}, {}),
        ToolPermission::Read,
        [&gateway](const QJsonObject&) { return ToolResult::ok(contextSummary(gateway)); }
    ));

    registry.add(makeTool(
        QStringLiteral("list_functions"),
        QStringLiteral("List analyzed functions in the binary."),
        toolSchema({}, {}),
        ToolPermission::Read,
        [&reader](const QJsonObject&) { return ToolResult::ok(reader.functions()); }
    ));

    registry.add(makeTool(
        QStringLiteral("list_strings"),
        QStringLiteral("List strings found in the binary."),
        toolSchema(
            {{QStringLiteral("limit"), integerProperty(QStringLiteral("Maximum strings to return"), 1, 2000)}},
            {}
        ),
        ToolPermission::Read,
        [&reader](const QJsonObject& arguments) {
            const int limit = ToolValidator::boundedInt(arguments, QStringLiteral("limit"), 200, 1, 2000);
            return ToolResult::ok(reader.strings(limit));
        }
    ));

    registry.add(makeTool(
        QStringLiteral("list_imports"),
        QStringLiteral("List imported symbols."),
        toolSchema({}, {}),
        ToolPermission::Read,
        [&reader](const QJsonObject&) { return ToolResult::ok(reader.imports()); }
    ));

    registry.add(makeTool(
        QStringLiteral("list_exports"),
        QStringLiteral("List exported symbols."),
        toolSchema({}, {}),
        ToolPermission::Read,
        [&reader](const QJsonObject&) { return ToolResult::ok(reader.exports()); }
    ));

    registry.add(makeTool(
        QStringLiteral("list_sections"),
        QStringLiteral("List binary sections."),
        toolSchema({}, {}),
        ToolPermission::Read,
        [&reader](const QJsonObject&) { return ToolResult::ok(reader.sections()); }
    ));

    registry.add(makeTool(
        QStringLiteral("disassemble"),
        QStringLiteral("Disassemble instructions at one or more addresses."),
        toolSchema(
            {
                {QStringLiteral("addresses"), addressesProperty(QStringLiteral("Address or list of addresses"))},
                {QStringLiteral("count"), integerProperty(QStringLiteral("Instruction count per address"), 1, 200)}
            },
            {QStringLiteral("addresses")}
        ),
        ToolPermission::Read,
        [&reader](const QJsonObject& arguments) {
            QVector<RVA> targets;
            QString error;
            if (!ToolValidator::addresses(arguments, targets, error)) {
                return ToolResult::fail(error);
            }
            const int count = ToolValidator::boundedInt(arguments, QStringLiteral("count"), 20, 1, 200);
            return ToolResult::ok(combineResults(targets, [&reader, count](RVA address) {
                return reader.disassemble(address, count);
            }));
        }
    ));

    registry.add(makeTool(
        QStringLiteral("disassemble_function"),
        QStringLiteral("Disassemble the whole function for one or more addresses."),
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
                return reader.disassembleFunction(address);
            }));
        }
    ));

    registry.add(makeTool(
        QStringLiteral("decompile_function"),
        QStringLiteral("Produce pseudo-C for the functions at one or more addresses."),
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
                return reader.decompile(address);
            }));
        }
    ));

    registry.add(makeTool(
        QStringLiteral("hexdump"),
        QStringLiteral("Hex dump bytes at one or more addresses."),
        toolSchema(
            {
                {QStringLiteral("addresses"), addressesProperty(QStringLiteral("Address or list of addresses"))},
                {QStringLiteral("length"), integerProperty(QStringLiteral("Byte count per address"), 1, 4096)}
            },
            {QStringLiteral("addresses")}
        ),
        ToolPermission::Read,
        [&reader](const QJsonObject& arguments) {
            QVector<RVA> targets;
            QString error;
            if (!ToolValidator::addresses(arguments, targets, error)) {
                return ToolResult::fail(error);
            }
            const int length = ToolValidator::boundedInt(arguments, QStringLiteral("length"), 64, 1, 4096);
            return ToolResult::ok(combineResults(targets, [&reader, length](RVA address) {
                return reader.hexdump(address, length);
            }));
        }
    ));

    registry.add(makeTool(
        QStringLiteral("read_bytes"),
        QStringLiteral("Read raw bytes at one or more addresses as hex."),
        toolSchema(
            {
                {QStringLiteral("addresses"), addressesProperty(QStringLiteral("Address or list of addresses"))},
                {QStringLiteral("length"), integerProperty(QStringLiteral("Byte count per address"), 1, 4096)}
            },
            {QStringLiteral("addresses")}
        ),
        ToolPermission::Read,
        [&reader](const QJsonObject& arguments) {
            QVector<RVA> targets;
            QString error;
            if (!ToolValidator::addresses(arguments, targets, error)) {
                return ToolResult::fail(error);
            }
            const int length = ToolValidator::boundedInt(arguments, QStringLiteral("length"), 64, 1, 4096);
            return ToolResult::ok(combineResults(targets, [&reader, length](RVA address) {
                return ToolValidator::hexBytes(reader.readBytes(address, length));
            }));
        }
    ));

    registry.add(makeTool(
        QStringLiteral("list_xrefs"),
        QStringLiteral("List cross references to one or more addresses."),
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
                return reader.xrefs(address);
            }));
        }
    ));
}

}
