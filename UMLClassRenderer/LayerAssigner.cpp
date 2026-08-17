#include "LayerAssigner.h"
#include <unordered_map>
#include <functional>

void assignLayers(ClassDiagram& diagram, const HierarchyInfo& hierarchy) {
    std::unordered_map<std::string, int> levelById;

    std::function<int(const std::string&)> computeLevel = [&](const std::string& id) -> int {
        auto cached = levelById.find(id);
        if (cached != levelById.end()) return cached->second;

        levelById[id] = 0; // temporary guard against accidental cycles

        int level = 0;
        auto parentsIt = hierarchy.parentsOf.find(id);
        if (parentsIt != hierarchy.parentsOf.end()) {
            for (const auto& parentId : parentsIt->second) {
                level = std::max(level, computeLevel(parentId) + 1);
            }
        }

        levelById[id] = level;
        return level;
        };

    for (auto& cls : diagram.classes) {
        cls.level = computeLevel(cls.id);
    }
}