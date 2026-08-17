#include "ActivitySwimlaneAnalyzer.h"
#include <iostream>
#include <unordered_map>
#include <algorithm>
#include <limits>

void analyzeSwimlanes(ActivityDiagram& diagram) {
    // Defensive pass only: membership was already resolved by Phase 2's
    // assignImplicitSwimlanes(), and orientation is whatever the XMI
    // parser set on Swimlane::horizontal. This guards against a node
    // referencing a swimlaneId that was never declared as a Swimlane —
    // rather than losing the node, we synthesize a fallback lane for it.
    for (auto& n : diagram.nodes) {
        if (diagram.findSwimlaneById(n.swimlaneId) == nullptr) {
            std::cerr << "[SwimlaneAnalysis] Node '" << n.id
                      << "' references unknown swimlane '" << n.swimlaneId
                      << "'; creating a fallback lane\n";
            Swimlane lane;
            lane.id = n.swimlaneId;
            lane.name = n.swimlaneId;
            lane.orderIndex = static_cast<int>(diagram.swimlanes.size());
            diagram.swimlanes.push_back(lane);
        }
    }
}

void finalizeSwimlaneOrder(ActivityDiagram& diagram)
{
    std::unordered_map<std::string, std::vector<double>> layersByLane;

    // Collect node layers per swimlane
    for (const auto& n : diagram.nodes)
    {
        if (n.layer >= 0)
        {
            layersByLane[n.swimlaneId]
                .push_back(static_cast<double>(n.layer));
        }
    }

    // Calculate average layer position for each swimlane
    auto avgLayer = [&](const Swimlane& lane) -> double
        {
            auto it = layersByLane.find(lane.id);

            if (it == layersByLane.end() || it->second.empty())
            {
                return std::numeric_limits<double>::max();
            }

            double sum = 0.0;

            for (double v : it->second)
                sum += v;

            return sum / static_cast<double>(it->second.size());
        };


    // Sort swimlanes according to their position in the workflow
    std::sort(
        diagram.swimlanes.begin(),
        diagram.swimlanes.end(),
        [&](const Swimlane& a, const Swimlane& b)
        {
            return avgLayer(a) < avgLayer(b);
        }
    );


    // Assign final order indexes
    for (int i = 0; i < static_cast<int>(diagram.swimlanes.size()); i++)
    {
        diagram.swimlanes[i].orderIndex = i;
    }
}