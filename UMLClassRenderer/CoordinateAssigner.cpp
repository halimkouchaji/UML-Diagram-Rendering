#include "CoordinateAssigner.h"
#include <map>
#include <vector>
#include <unordered_map>
#include <algorithm>

namespace {
    const double kHorizontalGap = 40.0;
    const double kVerticalGap = 60.0;
}

void assignCoordinates(ClassDiagram& diagram, const HierarchyInfo& hierarchy) {
    // Group classes by level, sorted by orderInLevel.
    std::map<int, std::vector<ClassNode*>> byLevel;
    for (auto& cls : diagram.classes) {
        byLevel[cls.level].push_back(&cls);
    }
    for (auto& [level, nodes] : byLevel) {
        std::sort(nodes.begin(), nodes.end(), [](ClassNode* a, ClassNode* b) {
            return a->orderInLevel < b->orderInLevel;
            });
    }

    // --- Pass 1: lay out x left-to-right within each level ---
    for (auto& [level, nodes] : byLevel) {
        double cursorX = 0.0;
        for (auto* node : nodes) {
            node->x = cursorX;
            cursorX += node->width + kHorizontalGap;
        }
    }

    // --- Pass 2: assign y top-to-bottom based on level, using max height per level ---
    std::map<int, double> maxHeightPerLevel;
    for (auto& [level, nodes] : byLevel) {
        double tallest = 0.0;
        for (auto* node : nodes) tallest = std::max(tallest, node->height);
        maxHeightPerLevel[level] = tallest;
    }

    std::unordered_map<int, double> yForLevel;
    double cursorY = 0.0;
    for (auto& [level, height] : maxHeightPerLevel) {
        yForLevel[level] = cursorY;
        cursorY += height + kVerticalGap;
    }
    for (auto& [level, nodes] : byLevel) {
        for (auto* node : nodes) node->y = yForLevel[level];
    }

    // --- Pass 3: nudge each parent to center horizontally over its children ---
    // Process from the deepest level upward so children are already final.
    std::unordered_map<std::string, ClassNode*> byId;
    for (auto& cls : diagram.classes) byId[cls.id] = &cls;

    std::vector<int> levelsDescending;
    for (auto& [level, _] : byLevel) levelsDescending.push_back(level);
    std::sort(levelsDescending.rbegin(), levelsDescending.rend());

    for (int level : levelsDescending) {
        for (auto& [parentId, children] : hierarchy.childrenOf) {
            auto parentIt = byId.find(parentId);
            if (parentIt == byId.end() || parentIt->second->level != level - 1) continue;
            if (children.empty()) continue;

            double minX = 1e18, maxX = -1e18;
            for (const auto& childId : children) {
                auto childIt = byId.find(childId);
                if (childIt == byId.end()) continue;
                minX = std::min(minX, childIt->second->x);
                maxX = std::max(maxX, childIt->second->x + childIt->second->width);
            }
            if (minX <= maxX) {
                double childrenMidpoint = (minX + maxX) / 2.0;
                parentIt->second->x = childrenMidpoint - parentIt->second->width / 2.0;
            }
        }
    }
}