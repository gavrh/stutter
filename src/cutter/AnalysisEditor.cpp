#include <cutter/AnalysisEditor.hpp>

#include <core/Cutter.h>

static QString hexAddress(RVA address) {
    return QStringLiteral("0x") + QString::number(address, 16);
}

static QString functionNameAt(RVA address) {
    const CutterJson info = Core()->cmdj(
        QStringLiteral("afij @ %1").arg(hexAddress(address))
    );
    return info[QStringLiteral("name")].toString();
}

static bool variableNamed(RVA functionAddress, const QString& name) {
    const CutterJson variables = Core()->cmdj(
        QStringLiteral("afvj @ %1").arg(hexAddress(functionAddress))
    );
    for (const CutterJson& variable : variables) {
        if (variable[QStringLiteral("name")].toString() == name) return true;
    }
    return false;
}

constexpr int instructionProbeBytes = 16;

bool AnalysisEditor::renameFunction(RVA address, const QString& name) {
    if (address == RVA_INVALID || name.isEmpty()) return false;
    const QString previous = functionNameAt(address);
    Core()->renameFunction(address, name);
    const QString current = functionNameAt(address);
    return !current.isEmpty() && (current == name || current != previous);
}

bool AnalysisEditor::createFunction(RVA address, const QString& name) {
    if (address == RVA_INVALID) return false;
    if (name.isEmpty()) {
        Core()->createFunctionAt(address);
    } else {
        Core()->createFunctionAt(address, name);
    }
    return Core()->getFunctionStart(address) != RVA_INVALID;
}

bool AnalysisEditor::deleteFunction(RVA address) {
    if (address == RVA_INVALID) return false;
    Core()->delFunction(address);
    return Core()->getFunctionStart(address) == RVA_INVALID;
}

bool AnalysisEditor::setComment(RVA address, const QString& comment) {
    if (address == RVA_INVALID || comment.isEmpty()) return false;
    Core()->setComment(address, comment);
    return Core()->getCommentAt(address) == comment;
}

bool AnalysisEditor::deleteComment(RVA address) {
    if (address == RVA_INVALID) return false;
    Core()->delComment(address);
    return Core()->getCommentAt(address).isEmpty();
}

bool AnalysisEditor::renameVariable(
    RVA functionAddress,
    const QString& oldName,
    const QString& newName
) {
    if (functionAddress == RVA_INVALID || oldName.isEmpty() || newName.isEmpty()) return false;
    Core()->renameFunctionVariable(newName, oldName, functionAddress);
    return variableNamed(functionAddress, newName) && !variableNamed(functionAddress, oldName);
}

bool AnalysisEditor::setInstruction(RVA address, const QString& assembly) {
    if (address == RVA_INVALID || assembly.isEmpty()) return false;
    if (!Core()->isWriteModeEnabled()) return false;
    const QByteArray before = Core()->ioRead(address, instructionProbeBytes);
    Core()->editBytes(address, assembly);
    const QByteArray after = Core()->ioRead(address, instructionProbeBytes);
    return before != after;
}
