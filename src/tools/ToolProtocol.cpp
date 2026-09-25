#include <tools/ToolProtocol.hpp>

#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QUuid>

namespace stutter {

static const QRegularExpression& blockPattern() {
    static const QRegularExpression pattern(
        QStringLiteral(R"(```stutter-tool\s*([\s\S]*?)\s*```)"),
        QRegularExpression::DotMatchesEverythingOption
    );
    return pattern;
}

QString toolProtocolInstructions(const QVector<ToolDefinition>& tools) {
    QString text = QStringLiteral(
        "## Cutter tools\n"
        "You can call Cutter tools to inspect or change the binary. To call a tool, reply with only a "
        "fenced block in this exact form:\n\n"
        "```stutter-tool\n{\"tool\": \"<name>\", \"arguments\": { }}\n```\n\n"
        "After the tool runs you will receive a message starting with \"Tool result:\". Then continue. "
        "Only call the tools listed below, and do not describe the block in prose. "
        "If repeated tool calls are not making progress, stop calling tools and explain what you tried "
        "and what is blocking you.\n\n"
        "Available tools:\n"
    );
    for (const ToolDefinition& tool : tools) {
        text += QStringLiteral("- %1: %2\n").arg(tool.name, tool.description);
        text += QStringLiteral("  arguments: %1\n").arg(
            QString::fromUtf8(QJsonDocument(tool.inputSchema).toJson(QJsonDocument::Compact))
        );
    }
    return text;
}

bool parseToolCall(const QString& text, ToolCall& out) {
    const QRegularExpressionMatch match = blockPattern().match(text);
    if (!match.hasMatch()) return false;

    const QByteArray body = match.captured(1).trimmed().toUtf8();
    const QJsonDocument document = QJsonDocument::fromJson(body);
    if (!document.isObject()) return false;

    const QJsonObject object = document.object();
    const QString name = object.value(QStringLiteral("tool")).toString();
    if (name.isEmpty()) return false;

    out.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    out.name = name;
    out.arguments = object.value(QStringLiteral("arguments")).toObject();
    out.rawArguments = QString::fromUtf8(body);
    return true;
}

QString stripToolBlocks(const QString& text) {
    QString result = text;
    result.remove(blockPattern());

    const int open = result.lastIndexOf(QStringLiteral("```stutter-tool"));
    if (open >= 0 && result.indexOf(QStringLiteral("```"), open + 3) < 0) {
        result.truncate(open);
    }
    return result;
}

}
