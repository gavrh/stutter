#pragma once

#include <core/CutterCommon.h>

#include <QString>

class AnalysisEditor {
public:
    bool renameFunction(RVA address, const QString& name);
    bool createFunction(RVA address, const QString& name);
    bool deleteFunction(RVA address);
    bool setComment(RVA address, const QString& comment);
    bool deleteComment(RVA address);
    bool renameVariable(RVA functionAddress, const QString& oldName, const QString& newName);
    bool setInstruction(RVA address, const QString& assembly);
};
