#include "ActivityEdgeRouter.h"
#include <algorithm>
#include <unordered_set>

namespace {

    Point makePoint(double x, double y) {
        Point p;
        p.x = x;
        p.y = y;
        return p;
    }

    double leftOf(const ActivityNode& n) { return n.x; }
    double rightOf(const ActivityNode& n) { return n.x + n.width; }
    double topOf(const ActivityNode& n) { return n.y; }
    double bottomOf(const ActivityNode& n) { return n.y + n.height; }
    double cxOf(const ActivityNode& n) { return n.x + n.width / 2.0; }
    double cyOf(const ActivityNode& n) { return n.y + n.height / 2.0; }

    const ActivityNode* findNode(const ActivityDiagram& diagram, const std::string& id) {
        for (const auto& node : diagram.nodes) {
            if (node.id == id) return &node;
        }
        return nullptr;
    }

    std::string labelForFlow(const ActivityFlow& flow) {
        if (!flow.guardLabel.empty()) return flow.guardLabel;
        if (!flow.objectLabel.empty()) return flow.objectLabel;
        return "";
    }

} // namespace

void routeActivityEdges(ActivityDiagram& diagram,
    ActivityLayoutContext& ctx,
    const FlowAnalysis& flowAnalysis) {
    ctx.edgeLabels.clear();

    std::unordered_set<std::string> backEdges = flowAnalysis.backEdgeFlowIds;

    for (auto& flow : diagram.flows) {
        const ActivityNode* from = findNode(diagram, flow.fromId);
        const ActivityNode* to = findNode(diagram, flow.toId);

        flow.points.clear();

        if (!from || !to) {
            ctx.warnings.push_back("Flow '" + flow.id + "' references a missing node.");
            continue;
        }

        flow.isBackEdge = backEdges.count(flow.id) > 0 || to->layer <= from->layer;

        if (flow.isBackEdge) {
            double loopX = std::max(rightOf(*from), rightOf(*to)) + 55.0;

            flow.points.push_back(makePoint(rightOf(*from), cyOf(*from)));
            flow.points.push_back(makePoint(loopX, cyOf(*from)));
            flow.points.push_back(makePoint(loopX, cyOf(*to)));
            flow.points.push_back(makePoint(cxOf(*to), topOf(*to)));
        }
        else if (from->layer == to->layer) {
            double midX = (cxOf(*from) + cxOf(*to)) / 2.0;

            if (cxOf(*from) <= cxOf(*to)) {
                flow.points.push_back(makePoint(rightOf(*from), cyOf(*from)));
                flow.points.push_back(makePoint(midX, cyOf(*from)));
                flow.points.push_back(makePoint(midX, cyOf(*to)));
                flow.points.push_back(makePoint(leftOf(*to), cyOf(*to)));
            }
            else {
                flow.points.push_back(makePoint(leftOf(*from), cyOf(*from)));
                flow.points.push_back(makePoint(midX, cyOf(*from)));
                flow.points.push_back(makePoint(midX, cyOf(*to)));
                flow.points.push_back(makePoint(rightOf(*to), cyOf(*to)));
            }
        }
        else {
            double midY = (bottomOf(*from) + topOf(*to)) / 2.0;

            flow.points.push_back(makePoint(cxOf(*from), bottomOf(*from)));
            flow.points.push_back(makePoint(cxOf(*from), midY));
            flow.points.push_back(makePoint(cxOf(*to), midY));
            flow.points.push_back(makePoint(cxOf(*to), topOf(*to)));
        }

        std::string label = labelForFlow(flow);
        if (!label.empty() && !flow.points.empty()) {
            const Point& a = flow.points[flow.points.size() / 2 - (flow.points.size() % 2 == 0 ? 1 : 0)];
            const Point& b = flow.points[flow.points.size() / 2];

            ActivityEdgeLabel edgeLabel;
            edgeLabel.flowId = flow.id;
            edgeLabel.text = label;
            edgeLabel.width = std::max(36.0, static_cast<double>(label.size()) * 7.0 + 12.0);
            edgeLabel.height = 18.0;
            edgeLabel.x = (a.x + b.x) / 2.0 - edgeLabel.width / 2.0;
            edgeLabel.y = (a.y + b.y) / 2.0 - edgeLabel.height - 4.0;

            ctx.edgeLabels.push_back(edgeLabel);
        }
    }
}