#pragma once

#include <providers/ProviderTypes.hpp>
#include <tools/ToolCall.hpp>

#include <QString>
#include <QVector>

namespace stutter {

QString toolProtocolInstructions(const QVector<ToolDefinition>& tools);
bool parseToolCall(const QString& text, ToolCall& out);
QString stripToolBlocks(const QString& text);

}
