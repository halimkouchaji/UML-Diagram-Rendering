#include "OverlapResolver.h"
#include <map>
#include <vector>
#include <algorithm>
#include <limits>

namespace {
    const double kMinGap = 20.0;
}

void resolveOverlapsAndNormalize(ClassDiagram& diagram) {
    // --- Step 1: resolve overlaps within each level ---
    std::map<int, std::vector<ClassNode*>> byLevel;
    for (auto& cls : diagram.classes) byLevel[cls.level].push_back(&cls);

    for (auto& [level, nodes] : byLevel) {
        std::sort(nodes.begin(), nodes.end(), [](ClassNode* a, ClassNode* b) {
            return a->x < b->x;
            });
        for (size_t i = 1; i < nodes.size(); ++i) {
            ClassNode* prev = nodes[i - 1];
            ClassNode* curr = nodes[i];
            double prevRight = prev->x + prev->width;
            if (curr->x < prevRight + kMinGap) {
                curr->x = prevRight + kMinGap;
            }
        }
    }

    // --- Step 2: find the minimum x / y across all boxes AND all edge points ---
    double minX = std::numeric_limits<double>::max();
    double minY = std::numeric_limits<double>::max();

    for (const auto& cls : diagram.classes) {
        minX = std::min(minX, cls.x);
        minY = std::min(minY, cls.y);
    }
    for (const auto& edge : diagram.edges) {
        for (const auto& p : edge.points) {
            minX = std::min(minX, p.x);
            minY = std::min(minY, p.y);
        }
    }

    if (minX == std::numeric_limits<double>::max()) return; // empty diagram, nothing to do

    // --- Step 3: shift everything so minX/minY become 0 ---
    for (auto& cls : diagram.classes) {
        cls.x -= minX;
        cls.y -= minY;
    }
    for (auto& edge : diagram.edges) {
        for (auto& p : edge.points) {
            p.x -= minX;
            p.y -= minY;
        }
    }
}