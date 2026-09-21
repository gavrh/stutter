#pragma once

#include <tools/Tool.hpp>

#include <QHash>
#include <QString>
#include <QVector>

class CutterGateway;

namespace stutter {

class ToolRegistry {
public:
    void add(Tool tool);
    bool contains(const QString& name) const;
    const Tool* find(const QString& name) const;
    QVector<ToolDefinition> definitions() const;
    QVector<Tool> tools() const;

private:
    QVector<Tool> tools_;
    QHash<QString, int> index_;
};

void registerAllTools(ToolRegistry& registry, CutterGateway& gateway);
void registerReadTools(ToolRegistry& registry, CutterGateway& gateway);
void registerAnalysisTools(ToolRegistry& registry, CutterGateway& gateway);
void registerMutationTools(ToolRegistry& registry, CutterGateway& gateway);
void registerDebugTools(ToolRegistry& registry, CutterGateway& gateway);

}
