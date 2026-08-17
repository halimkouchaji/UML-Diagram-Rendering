#include "LayerOrderer.h"
#include <unordered_map>
#include <map>
#include <vector>
#include <algorithm>

void assignOrder(ClassDiagram& diagram, const HierarchyInfo& hierarchy) {
    // Group class ids by level, preserving parse order as the initial order.
    std::map<int, std::vector<std::string>> byLevel;
    for (const auto& cls : diagram.classes) {
        byLevel[cls.level].push_back(cls.id);
    }

    // Level 0: order = simple left-to-right in the order encountered.
    std::unordered_map<std::string, double> barycenter;
    if (byLevel.count(0)) {
        auto& level0 = byLevel[0];
        for (size_t i = 0; i < level0.size(); ++i) {
            barycenter[level0[i]] = static_cast<double>(i);
        }
    }

    // For each deeper level, barycenter = average order-position of parent(s).
    for (auto& [level, ids] : byLevel) {
        if (level == 0) continue;

        for (const auto& id : ids) {
            auto parentIt = hierarchy.parentsOf.find(id);
            if (parentIt != hierarchy.parentsOf.end() && !parentIt->second.empty()
                && barycenter.count(parentIt->second[0])) {
                barycenter[id] = barycenter[parentIt->second[0]];
            }
            else {
                barycenter[id] = 0.0;
            }
        }

        // Sort this level's ids by barycenter value.
        std::sort(ids.begin(), ids.end(), [&](const std::string& a, const std::string& b) {
            return barycenter[a] < barycenter[b];
            });
    }

    // Write back the final order index into each ClassNode.
    std::unordered_map<std::string, int> orderById;
    for (auto& [level, ids] : byLevel) {
        for (size_t i = 0; i < ids.size(); ++i) {
            orderById[ids[i]] = static_cast<int>(i);
        }
    }

    for (auto& cls : diagram.classes) {
        cls.orderInLevel = orderById[cls.id];
    }
}