#include "ActivityLayoutCleaner.h"
#include "ActivityEdgeRouter.h"
#include <algorithm>
#include <map>
#include <unordered_map>

namespace {

    bool overlaps(const ActivityNode& a, const ActivityNode& b, double gap) {
        return !(a.x + a.width + gap <= b.x ||
            b.x + b.width + gap <= a.x ||
            a.y + a.height + gap <= b.y ||
            b.y + b.height + gap <= a.y);
    }

} // namespace

void cleanupActivityLayout(ActivityDiagram& diagram,
    ActivityLayoutContext& ctx,
    const FlowAnalysis& flowAnalysis) {
    constexpr double gap = 24.0;

    for (int iteration = 0; iteration < 4; ++iteration) {
        bool changed = false;

        std::map<std::pair<int, std::string>, std::vector<ActivityNode*>> groups;
        for (auto& node : diagram.nodes) {
            if (node.isSpacer) continue;
            groups[{node.layer, node.swimlaneId}].push_back(&node);
        }

        for (auto& entry : groups) {
            auto& nodes = entry.second;
            std::sort(nodes.begin(), nodes.end(),
                [](const ActivityNode* a, const ActivityNode* b) {
                    return a->x < b->x;
                });

            for (size_t i = 1; i < nodes.size(); ++i) {
                ActivityNode* prev = nodes[i - 1];
                ActivityNode* cur = nodes[i];

                if (overlaps(*prev, *cur, gap)) {
                    cur->x = prev->x + prev->width + gap;
                    changed = true;
                }
            }
        }

        for (auto& lane : diagram.swimlanes) {
            double maxRight = lane.x + lane.width;
            double maxBottom = lane.y + lane.height;

            for (const auto& node : diagram.nodes) {
                if (node.swimlaneId != lane.id || node.isSpacer) continue;

                maxRight = std::max(maxRight, node.x + node.width + ctx.swimlanePadding);
                maxBottom = std::max(maxBottom, node.y + node.height + ctx.swimlanePadding);
            }

            if (maxRight > lane.x + lane.width) {
                lane.width = maxRight - lane.x;
                changed = true;
            }

            if (maxBottom > lane.y + lane.height) {
                lane.height = maxBottom - lane.y;
                changed = true;
            }
        }

        double xCursor = ctx.padding;
        double yCursor = ctx.padding;

        bool horizontalLanes = false;
        for (const auto& lane : diagram.swimlanes) {
            if (lane.horizontal) {
                horizontalLanes = true;
                break;
            }
        }

        std::sort(diagram.swimlanes.begin(), diagram.swimlanes.end(),
            [](const Swimlane& a, const Swimlane& b) {
                return a.orderIndex < b.orderIndex;
            });

        if (!horizontalLanes) {
            for (auto& lane : diagram.swimlanes) {
                double dx = xCursor - lane.x;
                if (dx != 0.0) {
                    for (auto& node : diagram.nodes) {
                        if (node.swimlaneId == lane.id) node.x += dx;
                    }
                    lane.x = xCursor;
                }
                xCursor += lane.width;
            }
        }
        else {
            for (auto& lane : diagram.swimlanes) {
                double dy = yCursor - lane.y;
                if (dy != 0.0) {
                    for (auto& node : diagram.nodes) {
                        if (node.swimlaneId == lane.id) node.y += dy;
                    }
                    lane.y = yCursor;
                }
                yCursor += lane.height;
            }
        }

        if (!changed) break;
    }

    double maxRight = 0.0;
    double maxBottom = 0.0;

    for (const auto& lane : diagram.swimlanes) {
        maxRight = std::max(maxRight, lane.x + lane.width);
        maxBottom = std::max(maxBottom, lane.y + lane.height);
    }

    for (const auto& node : diagram.nodes) {
        if (node.isSpacer) continue;
        maxRight = std::max(maxRight, node.x + node.width);
        maxBottom = std::max(maxBottom, node.y + node.height);
    }

    ctx.diagramWidth = maxRight + ctx.padding;
    ctx.diagramHeight = maxBottom + ctx.padding;

    routeActivityEdges(diagram, ctx, flowAnalysis);
}