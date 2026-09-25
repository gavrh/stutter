#include <cutter/CutterGateway.hpp>
#include <tools/ToolRegistry.hpp>
#include <tools/ToolValidator.hpp>

#include <QByteArray>
#include <QJsonArray>
#include <QJsonObject>
#include <QStringList>

struct RenameItem {
    RVA address = RVA_INVALID;
    QString name;
};

struct CommentItem {
    RVA address = RVA_INVALID;
    QString comment;
};

static bool parseRenames(const QJsonObject& arguments, QVector<RenameItem>& out, QString& error) {
    const QJsonValue renames = arguments.value(QStringLiteral("renames"));
    if (renames.isArray()) {
        for (const QJsonValue& value : renames.toArray()) {
            const QJsonObject item = value.toObject();
            RenameItem entry;
            if (!stutter::ToolValidator::addressValue(item.value(QStringLiteral("address")), entry.address, error)) {
                return false;
            }
            if (!stutter::ToolValidator::requiredString(item, QStringLiteral("name"), entry.name, error)) {
                return false;
            }
            out.append(entry);
        }
        if (out.isEmpty()) {
            error = QStringLiteral("renames is empty");
            return false;
        }
        return true;
    }
    RenameItem entry;
    if (!stutter::ToolValidator::address(arguments, QStringLiteral("address"), entry.address, error)) return false;
    if (!stutter::ToolValidator::requiredString(arguments, QStringLiteral("name"), entry.name, error)) return false;
    out.append(entry);
    return true;
}

static bool parseComments(const QJsonObject& arguments, QVector<CommentItem>& out, QString& error) {
    const QJsonValue comments = arguments.value(QStringLiteral("comments"));
    if (comments.isArray()) {
        for (const QJsonValue& value : comments.toArray()) {
            const QJsonObject item = value.toObject();
            CommentItem entry;
            if (!stutter::ToolValidator::addressValue(item.value(QStringLiteral("address")), entry.address, error)) {
                return false;
            }
            if (!stutter::ToolValidator::requiredString(item, QStringLiteral("comment"), entry.comment, error)) {
                return false;
            }
            out.append(entry);
        }
        if (out.isEmpty()) {
            error = QStringLiteral("comments is empty");
            return false;
        }
        return true;
    }
    CommentItem entry;
    if (!stutter::ToolValidator::address(arguments, QStringLiteral("address"), entry.address, error)) return false;
    if (!stutter::ToolValidator::requiredString(arguments, QStringLiteral("comment"), entry.comment, error)) return false;
    out.append(entry);
    return true;
}

namespace stutter {

void registerMutationTools(ToolRegistry& registry, CutterGateway& gateway) {
    AnalysisEditor& editor = gateway.editor();
    BinaryEditor& binaryEditor = gateway.binaryEditor();

    registry.add(makeTool(
        QStringLiteral("rename_function"),
        QStringLiteral("Rename one or more functions, each given an address and a new name."),
        toolSchema(
            {
                {QStringLiteral("renames"), QJsonObject {
                    {QStringLiteral("type"), QStringLiteral("array")},
                    {QStringLiteral("description"), QStringLiteral("List of {address, name} entries")},
                    {QStringLiteral("items"), QJsonObject {
                        {QStringLiteral("type"), QStringLiteral("object")},
                        {QStringLiteral("properties"), QJsonObject {
                            {QStringLiteral("address"), addressesProperty(QStringLiteral("Function address"))},
                            {QStringLiteral("name"), stringProperty(QStringLiteral("New function name"))}
                        }},
                        {QStringLiteral("required"), QJsonArray {
                            QStringLiteral("address"), QStringLiteral("name")
                        }}
                    }}
                }},
                {QStringLiteral("address"), addressesProperty(QStringLiteral("Single function address"))},
                {QStringLiteral("name"), stringProperty(QStringLiteral("Single new function name"))}
            },
            {}
        ),
        ToolPermission::Analysis,
        [&editor](const QJsonObject& arguments) {
            QVector<RenameItem> renames;
            QString error;
            if (!parseRenames(arguments, renames, error)) return ToolResult::fail(error);
            QStringList results;
            for (const RenameItem& rename : renames) {
                if (!editor.renameFunction(rename.address, rename.name)) {
                    results.append(QStringLiteral("Failed to rename 0x%1").arg(rename.address, 0, 16));
                    continue;
                }
                results.append(QStringLiteral("Renamed 0x%1 to %2")
                    .arg(rename.address, 0, 16).arg(rename.name));
            }
            return ToolResult::ok(results.join(QLatin1Char('\n')));
        }
    ));

    registry.add(makeTool(
        QStringLiteral("create_function"),
        QStringLiteral("Create a function at an address."),
        toolSchema(
            {
                {QStringLiteral("address"), addressesProperty(QStringLiteral("Function entry address"))},
                {QStringLiteral("name"), stringProperty(QStringLiteral("Optional function name"))}
            },
            {QStringLiteral("address")}
        ),
        ToolPermission::Analysis,
        [&editor](const QJsonObject& arguments) {
            RVA address = RVA_INVALID;
            QString error;
            if (!stutter::ToolValidator::address(arguments, QStringLiteral("address"), address, error)) {
                return ToolResult::fail(error);
            }
            const QString name = stutter::ToolValidator::string(arguments, QStringLiteral("name"));
            if (!editor.createFunction(address, name)) {
                return ToolResult::fail(QStringLiteral("Unable to create function"));
            }
            return ToolResult::ok(QStringLiteral("Created function at 0x%1").arg(address, 0, 16));
        }
    ));

    registry.add(makeTool(
        QStringLiteral("delete_function"),
        QStringLiteral("Delete one or more functions by address."),
        toolSchema(
            {{QStringLiteral("addresses"), addressesProperty(QStringLiteral("Address or list of addresses"))}},
            {QStringLiteral("addresses")}
        ),
        ToolPermission::Analysis,
        [&editor](const QJsonObject& arguments) {
            QVector<RVA> targets;
            QString error;
            if (!stutter::ToolValidator::addresses(arguments, targets, error)) {
                return ToolResult::fail(error);
            }
            QStringList results;
            for (RVA address : targets) {
                results.append(editor.deleteFunction(address)
                    ? QStringLiteral("Deleted function at 0x%1").arg(address, 0, 16)
                    : QStringLiteral("Failed to delete 0x%1").arg(address, 0, 16));
            }
            return ToolResult::ok(results.join(QLatin1Char('\n')));
        }
    ));

    registry.add(makeTool(
        QStringLiteral("set_comment"),
        QStringLiteral("Set comments for one or more addresses."),
        toolSchema(
            {
                {QStringLiteral("comments"), QJsonObject {
                    {QStringLiteral("type"), QStringLiteral("array")},
                    {QStringLiteral("description"), QStringLiteral("List of {address, comment} entries")},
                    {QStringLiteral("items"), QJsonObject {
                        {QStringLiteral("type"), QStringLiteral("object")},
                        {QStringLiteral("properties"), QJsonObject {
                            {QStringLiteral("address"), addressesProperty(QStringLiteral("Address to comment"))},
                            {QStringLiteral("comment"), stringProperty(QStringLiteral("Comment text"))}
                        }},
                        {QStringLiteral("required"), QJsonArray {
                            QStringLiteral("address"), QStringLiteral("comment")
                        }}
                    }}
                }},
                {QStringLiteral("address"), addressesProperty(QStringLiteral("Single address"))},
                {QStringLiteral("comment"), stringProperty(QStringLiteral("Single comment text"))}
            },
            {}
        ),
        ToolPermission::Analysis,
        [&editor](const QJsonObject& arguments) {
            QVector<CommentItem> comments;
            QString error;
            if (!parseComments(arguments, comments, error)) return ToolResult::fail(error);
            QStringList results;
            for (const CommentItem& comment : comments) {
                results.append(editor.setComment(comment.address, comment.comment)
                    ? QStringLiteral("Set comment at 0x%1").arg(comment.address, 0, 16)
                    : QStringLiteral("Failed to comment 0x%1").arg(comment.address, 0, 16));
            }
            return ToolResult::ok(results.join(QLatin1Char('\n')));
        }
    ));

    registry.add(makeTool(
        QStringLiteral("delete_comment"),
        QStringLiteral("Delete the comment at one or more addresses."),
        toolSchema(
            {{QStringLiteral("addresses"), addressesProperty(QStringLiteral("Address or list of addresses"))}},
            {QStringLiteral("addresses")}
        ),
        ToolPermission::Analysis,
        [&editor](const QJsonObject& arguments) {
            QVector<RVA> targets;
            QString error;
            if (!stutter::ToolValidator::addresses(arguments, targets, error)) {
                return ToolResult::fail(error);
            }
            QStringList results;
            for (RVA address : targets) {
                results.append(editor.deleteComment(address)
                    ? QStringLiteral("Deleted comment at 0x%1").arg(address, 0, 16)
                    : QStringLiteral("Failed to delete comment at 0x%1").arg(address, 0, 16));
            }
            return ToolResult::ok(results.join(QLatin1Char('\n')));
        }
    ));

    registry.add(makeTool(
        QStringLiteral("rename_variable"),
        QStringLiteral("Rename a local variable in a function."),
        toolSchema(
            {
                {QStringLiteral("function_address"), addressesProperty(QStringLiteral("Function address"))},
                {QStringLiteral("old_name"), stringProperty(QStringLiteral("Current variable name"))},
                {QStringLiteral("new_name"), stringProperty(QStringLiteral("New variable name"))}
            },
            {QStringLiteral("function_address"), QStringLiteral("old_name"), QStringLiteral("new_name")}
        ),
        ToolPermission::Analysis,
        [&editor](const QJsonObject& arguments) {
            RVA address = RVA_INVALID;
            QString oldName;
            QString newName;
            QString error;
            if (!stutter::ToolValidator::address(arguments, QStringLiteral("function_address"), address, error)) {
                return ToolResult::fail(error);
            }
            if (!stutter::ToolValidator::requiredString(arguments, QStringLiteral("old_name"), oldName, error)) {
                return ToolResult::fail(error);
            }
            if (!stutter::ToolValidator::requiredString(arguments, QStringLiteral("new_name"), newName, error)) {
                return ToolResult::fail(error);
            }
            if (!editor.renameVariable(address, oldName, newName)) {
                return ToolResult::fail(QStringLiteral("Unable to rename variable"));
            }
            return ToolResult::ok(QStringLiteral("Renamed %1 to %2").arg(oldName, newName));
        }
    ));

    registry.add(makeTool(
        QStringLiteral("set_instruction"),
        QStringLiteral("Assemble and write an instruction at an address (write mode required)."),
        toolSchema(
            {
                {QStringLiteral("address"), addressesProperty(QStringLiteral("Address to patch"))},
                {QStringLiteral("assembly"), stringProperty(QStringLiteral("Assembly text"))}
            },
            {QStringLiteral("address"), QStringLiteral("assembly")}
        ),
        ToolPermission::Binary,
        [&editor](const QJsonObject& arguments) {
            RVA address = RVA_INVALID;
            QString assembly;
            QString error;
            if (!stutter::ToolValidator::address(arguments, QStringLiteral("address"), address, error)) {
                return ToolResult::fail(error);
            }
            if (!stutter::ToolValidator::requiredString(arguments, QStringLiteral("assembly"), assembly, error)) {
                return ToolResult::fail(error);
            }
            if (!editor.setInstruction(address, assembly)) {
                return ToolResult::fail(QStringLiteral("Unable to write instruction; enable write mode"));
            }
            return ToolResult::ok(QStringLiteral("Wrote instruction at 0x%1").arg(address, 0, 16));
        }
    ));

    registry.add(makeTool(
        QStringLiteral("stage_patch"),
        QStringLiteral("Stage raw bytes to be committed to the binary."),
        toolSchema(
            {
                {QStringLiteral("address"), addressesProperty(QStringLiteral("Patch address"))},
                {QStringLiteral("hex"), stringProperty(QStringLiteral("Hex bytes, for example 90 90"))}
            },
            {QStringLiteral("address"), QStringLiteral("hex")}
        ),
        ToolPermission::Binary,
        [&binaryEditor](const QJsonObject& arguments) {
            RVA address = RVA_INVALID;
            QString error;
            if (!stutter::ToolValidator::address(arguments, QStringLiteral("address"), address, error)) {
                return ToolResult::fail(error);
            }
            QString hex = stutter::ToolValidator::string(arguments, QStringLiteral("hex"));
            hex.remove(QLatin1Char(' '));
            const QByteArray bytes = QByteArray::fromHex(hex.toLatin1());
            if (bytes.isEmpty()) {
                return ToolResult::fail(QStringLiteral("hex is empty or invalid"));
            }
            binaryEditor.stage(address, bytes);
            return ToolResult::ok(QStringLiteral("Staged %1 bytes at 0x%2")
                .arg(bytes.size()).arg(address, 0, 16));
        }
    ));

    registry.add(makeTool(
        QStringLiteral("commit_patches"),
        QStringLiteral("Commit all staged patches (write mode required)."),
        toolSchema({}, {}),
        ToolPermission::Binary,
        [&binaryEditor](const QJsonObject&) {
            if (!binaryEditor.commit()) {
                return ToolResult::fail(QStringLiteral("Unable to commit; enable write mode"));
            }
            return ToolResult::ok(QStringLiteral("Committed staged patches"));
        }
    ));

    registry.add(makeTool(
        QStringLiteral("discard_patches"),
        QStringLiteral("Discard all staged patches."),
        toolSchema({}, {}),
        ToolPermission::Binary,
        [&binaryEditor](const QJsonObject&) {
            binaryEditor.discard();
            return ToolResult::ok(QStringLiteral("Discarded staged patches"));
        }
    ));

    registry.add(makeTool(
        QStringLiteral("set_write_mode"),
        QStringLiteral("Enable or disable binary write mode."),
        toolSchema(
            {{QStringLiteral("enabled"), QJsonObject {
                {QStringLiteral("type"), QStringLiteral("boolean")},
                {QStringLiteral("description"), QStringLiteral("Whether write mode is enabled")}
            }}},
            {QStringLiteral("enabled")}
        ),
        ToolPermission::Binary,
        [&binaryEditor](const QJsonObject& arguments) {
            const QJsonValue value = arguments.value(QStringLiteral("enabled"));
            if (!value.isBool()) {
                return ToolResult::fail(QStringLiteral("enabled must be a boolean"));
            }
            const bool enabled = value.toBool();
            binaryEditor.setWriteMode(enabled);
            return ToolResult::ok(QStringLiteral("Write mode %1")
                .arg(enabled ? QStringLiteral("enabled") : QStringLiteral("disabled")));
        }
    ));
}

}
