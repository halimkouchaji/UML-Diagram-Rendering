#include "ActivityGraphBuilder.h"
#include <iostream>
#include <algorithm>

namespace {
    constexpr const char* kUnassignedLaneId = "__unassigned__";
}

void assignImplicitSwimlanes(ActivityDiagram& diagram) {
    bool needsUnassignedLane = false;
    for (const auto& n : diagram.nodes) {
        if (n.swimlaneId.empty()) {
            needsUnassignedLane = true;
            break;
        }
    }

    if (needsUnassignedLane && diagram.findSwimlaneById(kUnassignedLaneId) == nullptr) {
        Swimlane lane;
        lane.id = kUnassignedLaneId;
        lane.name = "";              // rendered without a header label
        lane.orderIndex = static_cast<int>(diagram.swimlanes.size());
        diagram.swimlanes.push_back(lane);
    }

    for (auto& n : diagram.nodes) {
        if (n.swimlaneId.empty()) {
            n.swimlaneId = kUnassignedLaneId;
        }
    }
}

ActivityGraph buildActivityGraph(const ActivityDiagram& diagram) {
    ActivityGraph graph;

    // Pre-seed every node with an empty entry so callers can safely
    // index graph.outgoing[id] / graph.incoming[id] without checking
    // for existence first (same convenience as the Class Diagram
    // HierarchyBuilder's adjacency maps).
    for (const auto& n : diagram.nodes) {
        graph.outgoing[n.id];
        graph.incoming[n.id];
    }

    for (const auto& f : diagram.flows) {
        graph.outgoing[f.fromId].push_back(f.id);
        graph.incoming[f.toId].push_back(f.id);
    }

    return graph;
}

bool validateActivityGraph(const ActivityDiagram& diagram, const ActivityGraph& graph) {
    bool ok = true;

    auto flowById = [&](const std::string& flowId) -> const ActivityFlow* {
        for (const auto& f : diagram.flows) {
            if (f.id == flowId) return &f;
        }
        return nullptr;
        };

    // Dangling references
    for (const auto& f : diagram.flows) {
        if (diagram.nodes.end() == std::find_if(diagram.nodes.begin(), diagram.nodes.end(),
            [&](const ActivityNode& n) { return n.id == f.fromId; })) {
            std::cerr << "[ActivityGraph] Flow '" << f.id << "' has unknown fromId '" << f.fromId << "'\n";
            ok = false;
        }
        if (diagram.nodes.end() == std::find_if(diagram.nodes.begin(), diagram.nodes.end(),
            [&](const ActivityNode& n) { return n.id == f.toId; })) {
            std::cerr << "[ActivityGraph] Flow '" << f.id << "' has unknown toId '" << f.toId << "'\n";
            ok = false;
        }
    }

    bool sawInitial = false;

    for (const auto& n : diagram.nodes) {
        auto outCount = graph.outgoing.count(n.id) ? graph.outgoing.at(n.id).size() : 0;
        auto inCount = graph.incoming.count(n.id) ? graph.incoming.at(n.id).size() : 0;

        switch (n.kind) {
        case ActivityNodeKind::Initial:
            sawInitial = true;
            break;
        case ActivityNodeKind::Decision:
            if (outCount < 2) {
                std::cerr << "[ActivityGraph] Decision node '" << n.id
                    << "' has only " << outCount << " outgoing flow(s), expected >= 2\n";
                ok = false;
            }
            break;
        case ActivityNodeKind::Merge:
            if (inCount < 2) {
                std::cerr << "[ActivityGraph] Merge node '" << n.id
                    << "' has only " << inCount << " incoming flow(s), expected >= 2\n";
                ok = false;
            }
            break;
        case ActivityNodeKind::Fork:
            if (outCount < 2) {
                std::cerr << "[ActivityGraph] Fork node '" << n.id
                    << "' has only " << outCount << " outgoing flow(s), expected >= 2\n";
                ok = false;
            }
            break;
        case ActivityNodeKind::Join:
            if (inCount < 2) {
                std::cerr << "[ActivityGraph] Join node '" << n.id
                    << "' has only " << inCount << " incoming flow(s), expected >= 2\n";
                ok = false;
            }
            break;
        default:
            break;
        }
    }

    if (!sawInitial) {
        std::cerr << "[ActivityGraph] No InitialNode found in diagram\n";
        ok = false;
    }

    return ok;
}