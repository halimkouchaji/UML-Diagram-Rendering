#include "ActivityCoordinateAssigner.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <unordered_map>

namespace {

    double textWidth(const std::string& text) {
        return std::max(40.0, static_cast<double>(text.size()) * 7.5);
    }

    void sizeNode(ActivityNode& n) {
        if (n.isSpacer) {
            n.width = 26.0;
            n.height = 26.0;
            return;
        }

        switch (n.kind) {
        case ActivityNodeKind::Initial:
            n.width = 24.0;
            n.height = 24.0;
            break;

        case ActivityNodeKind::ActivityFinal:
        case ActivityNodeKind::FlowFinal:
            n.width = 32.0;
            n.height = 32.0;
            break;

        case ActivityNodeKind::Decision:
        case ActivityNodeKind::Merge:
            n.width = 72.0;
            n.height = 56.0;
            break;

        case ActivityNodeKind::Fork:
        case ActivityNodeKind::Join:
            if (n.horizontalBar) {
                n.width = 120.0;
                n.height = 12.0;
            }
            else {
                n.width = 12.0;
                n.height = 120.0;
            }
            break;

        case ActivityNodeKind::Object:
            n.width = std::max(110.0, textWidth(n.name) + 32.0);
            n.height = 48.0;
            break;

        case ActivityNodeKind::Action:
        default:
            n.width = std::max(120.0, textWidth(n.name) + 36.0);
            n.height = 54.0;
            break;
        }
    }

} // namespace

ActivityLayoutContext assignActivityCoordinates(ActivityDiagram& diagram) {
    ActivityLayoutContext ctx;

    if (diagram.nodes.empty()) {
        ctx.diagramWidth = ctx.padding * 2.0;
        ctx.diagramHeight = ctx.padding * 2.0;
        return ctx;
    }

    for (auto& node : diagram.nodes) {
        sizeNode(node);
    }

    if (diagram.swimlanes.empty()) {
        Swimlane lane;
        lane.id = "__unassigned__";
        lane.name = "";
        lane.orderIndex = 0;
        diagram.swimlanes.push_back(lane);

        for (auto& node : diagram.nodes) {
            if (node.swimlaneId.empty()) {
                node.swimlaneId = lane.id;
            }
        }
    }

    std::sort(diagram.swimlanes.begin(), diagram.swimlanes.end(),
        [](const Swimlane& a, const Swimlane& b) {
            return a.orderIndex < b.orderIndex;
        });

    bool horizontalLanes = false;
    for (const auto& lane : diagram.swimlanes) {
        if (lane.horizontal) {
            horizontalLanes = true;
            break;
        }
    }

    std::unordered_map<std::string, int> maxOrderByLane;
    std::unordered_map<std::string, double> maxNodeWidthByLane;
    std::unordered_map<std::string, double> maxNodeHeightByLane;

    int maxLayer = 0;
    for (const auto& node : diagram.nodes) {
        maxLayer = std::max(maxLayer, node.layer);
        maxOrderByLane[node.swimlaneId] = std::max(maxOrderByLane[node.swimlaneId], node.orderInLayer);
        maxNodeWidthByLane[node.swimlaneId] = std::max(maxNodeWidthByLane[node.swimlaneId], node.width);
        maxNodeHeightByLane[node.swimlaneId] = std::max(maxNodeHeightByLane[node.swimlaneId], node.height);
    }

    if (!horizontalLanes) {
        double xCursor = ctx.padding;
        double totalHeight = ctx.padding * 2.0
            + ctx.laneHeaderSize
            + static_cast<double>(maxLayer + 1) * ctx.layerSpacing
            + 100.0;

        for (auto& lane : diagram.swimlanes) {
            int maxOrder = std::max(0, maxOrderByLane[lane.id]);
            double requiredWidth =
                ctx.swimlanePadding * 2.0
                + static_cast<double>(maxOrder + 1) * ctx.nodeSpacing
                + maxNodeWidthByLane[lane.id];

            lane.x = xCursor;
            lane.y = ctx.padding;
            lane.width = std::max(180.0, requiredWidth);
            lane.height = totalHeight;

            xCursor += lane.width;
        }

        for (auto& node : diagram.nodes) {
            Swimlane* lane = diagram.findSwimlaneById(node.swimlaneId);
            if (!lane) continue;

            double baseX = lane->x + ctx.swimlanePadding
                + static_cast<double>(std::max(0, node.orderInLayer)) * ctx.nodeSpacing;

            double generatedX = baseX + (maxNodeWidthByLane[node.swimlaneId] - node.width) / 2.0;
            double generatedY = lane->y + ctx.laneHeaderSize
                + ctx.swimlanePadding
                + static_cast<double>(std::max(0, node.layer)) * ctx.layerSpacing;

            if (node.hasXmiCoords) {
                node.x = generatedX * 0.70 + node.xmiX * 0.30;
                node.y = generatedY * 0.85 + node.xmiY * 0.15;
            }
            else {
                node.x = generatedX;
                node.y = generatedY;
            }

            node.x = std::max(node.x, lane->x + ctx.swimlanePadding);
            node.x = std::min(node.x, lane->x + lane->width - ctx.swimlanePadding - node.width);
            node.y = std::max(node.y, lane->y + ctx.laneHeaderSize + ctx.swimlanePadding);
        }

        ctx.diagramWidth = xCursor + ctx.padding;
        ctx.diagramHeight = totalHeight + ctx.padding;
    }
    else {
        double yCursor = ctx.padding;
        double totalWidth = ctx.padding * 2.0
            + static_cast<double>(maxLayer + 1) * ctx.layerSpacing
            + 180.0;

        for (auto& lane : diagram.swimlanes) {
            int maxOrder = std::max(0, maxOrderByLane[lane.id]);
            double requiredHeight =
                ctx.swimlanePadding * 2.0
                + ctx.laneHeaderSize
                + static_cast<double>(maxOrder + 1) * ctx.nodeSpacing
                + maxNodeHeightByLane[lane.id];

            lane.x = ctx.padding;
            lane.y = yCursor;
            lane.width = totalWidth;
            lane.height = std::max(150.0, requiredHeight);

            yCursor += lane.height;
        }

        for (auto& node : diagram.nodes) {
            Swimlane* lane = diagram.findSwimlaneById(node.swimlaneId);
            if (!lane) continue;

            node.x = lane->x + ctx.swimlanePadding
                + static_cast<double>(std::max(0, node.layer)) * ctx.layerSpacing;

            node.y = lane->y + ctx.laneHeaderSize + ctx.swimlanePadding
                + static_cast<double>(std::max(0, node.orderInLayer)) * ctx.nodeSpacing;

            if (node.hasXmiCoords) {
                node.x = node.x * 0.70 + node.xmiX * 0.30;
                node.y = node.y * 0.70 + node.xmiY * 0.30;
            }
        }

        ctx.diagramWidth = totalWidth + ctx.padding;
        ctx.diagramHeight = yCursor + ctx.padding;
    }

    return ctx;
}