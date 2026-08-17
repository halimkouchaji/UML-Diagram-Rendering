#include "HierarchyBuilder.h"

HierarchyInfo buildHierarchy(const ClassDiagram& diagram) {
    HierarchyInfo info;

    for (const auto& edge : diagram.edges) {
        if (edge.type != EdgeType::Generalization && edge.type != EdgeType::Realization) continue;

        const std::string& child = edge.fromId;   // the class
        const std::string& parent = edge.toId;    // the superclass OR interface

        info.parentsOf[child].push_back(parent);
        info.childrenOf[parent].push_back(child);
    }

    for (const auto& cls : diagram.classes) {
        if (info.parentsOf.find(cls.id) == info.parentsOf.end()) {
            info.roots.push_back(cls.id);
        }
    }

    return info;
}