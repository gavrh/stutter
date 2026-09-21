#include <tools/ToolRegistry.hpp>

namespace stutter {

void ToolRegistry::add(Tool tool) {
    const auto existing = index_.constFind(tool.name);
    if (existing != index_.constEnd()) {
        tools_[existing.value()] = std::move(tool);
        return;
    }
    index_.insert(tool.name, tools_.size());
    tools_.append(std::move(tool));
}

bool ToolRegistry::contains(const QString& name) const {
    return index_.contains(name);
}

const Tool* ToolRegistry::find(const QString& name) const {
    const auto existing = index_.constFind(name);
    if (existing == index_.constEnd()) return nullptr;
    return &tools_.at(existing.value());
}

QVector<ToolDefinition> ToolRegistry::definitions() const {
    QVector<ToolDefinition> definitions;
    definitions.reserve(tools_.size());
    for (const Tool& tool : tools_) {
        definitions.append(tool.definition());
    }
    return definitions;
}

QVector<Tool> ToolRegistry::tools() const {
    return tools_;
}

void registerAllTools(ToolRegistry& registry, CutterGateway& gateway) {
    registerReadTools(registry, gateway);
    registerAnalysisTools(registry, gateway);
    registerMutationTools(registry, gateway);
    registerDebugTools(registry, gateway);
}

}
