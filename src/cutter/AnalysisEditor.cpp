#include <cutter/AnalysisEditor.hpp>

#include <core/Cutter.h>

bool AnalysisEditor::renameFunction(RVA address, const QString& name) {
    if (address == RVA_INVALID || name.isEmpty()) return false;
    Core()->renameFunction(address, name);
    return true;
}

bool AnalysisEditor::createFunction(RVA address, const QString& name) {
    if (address == RVA_INVALID) return false;
    if (name.isEmpty()) {
        Core()->createFunctionAt(address);
    } else {
        Core()->createFunctionAt(address, name);
    }
    return true;
}

bool AnalysisEditor::deleteFunction(RVA address) {
    if (address == RVA_INVALID) return false;
    Core()->delFunction(address);
    return true;
}

bool AnalysisEditor::setComment(RVA address, const QString& comment) {
    if (address == RVA_INVALID) return false;
    Core()->setComment(address, comment);
    return true;
}

bool AnalysisEditor::deleteComment(RVA address) {
    if (address == RVA_INVALID) return false;
    Core()->delComment(address);
    return true;
}

bool AnalysisEditor::renameVariable(
    RVA functionAddress,
    const QString& oldName,
    const QString& newName
) {
    if (functionAddress == RVA_INVALID || oldName.isEmpty() || newName.isEmpty()) return false;
    Core()->renameFunctionVariable(newName, oldName, functionAddress);
    return true;
}

bool AnalysisEditor::setInstruction(RVA address, const QString& assembly) {
    if (address == RVA_INVALID || assembly.isEmpty()) return false;
    if (!Core()->isWriteModeEnabled()) return false;
    Core()->editBytes(address, assembly);
    return true;
}
