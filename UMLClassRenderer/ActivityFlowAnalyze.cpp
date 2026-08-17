#include "ActivityFlowAnalyzer.h"
#include <iostream>
#include <unordered_map>
#include <queue>
#include <functional>
#include <algorithm>
#include <limits>

namespace {

    std::unordered_map<std::string, const ActivityFlow*> buildFlowLookup(const ActivityDiagram& diagram) {
        std::unordered_map<std::string, const ActivityFlow*> lookup;
        for (const auto& f : diagram.flows) lookup[f.id] = &f;
        return lookup;
    }

    // BFS distance map from a single branch-start node, following outgoing
    // flows only, skipping any flow already classified as a back edge.
    // Distances are in "hops", branchStart itself is distance 0.
    std::unordered_map<std::string, int> bfsDistances(
        const std::string& start,
        const ActivityGraph& graph,
        const std::unordered_map<std::string, const ActivityFlow*>& flowLookup,
        const std::unordered_set<std::string>& backEdges)
    {
        std::unordered_map<std::string, int> dist;
        std::queue<std::string> q;
        dist[start] = 0;
        q.push(start);
        while (!q.empty()) {
            std::string u = q.front(); q.pop();
            auto it = graph.outgoing.find(u);
            if (it == graph.outgoing.end()) continue;
            for (const auto& flowId : it->second) {
                if (backEdges.count(flowId)) continue;
                const ActivityFlow* f = flowLookup.at(flowId);
                if (!dist.count(f->toId)) {
                    dist[f->toId] = dist[u] + 1;
                    q.push(f->toId);
                }
            }
        }
        return dist;
    }

    // Finds the nearest node reachable from every branch start (excluding the
    // branch starts themselves being compared against each other — a shared
    // start would trivially "converge" at distance 0, which we don't want).
    // Returns empty string if no common convergence point exists.
    std::string findNearestConvergence(
        const std::vector<std::string>& branchStarts,
        const ActivityGraph& graph,
        const std::unordered_map<std::string, const ActivityFlow*>& flowLookup,
        const std::unordered_set<std::string>& backEdges)
    {
        if (branchStarts.size() < 2) return "";

        std::vector<std::unordered_map<std::string, int>> perBranch;
        perBranch.reserve(branchStarts.size());
        for (const auto& start : branchStarts) {
            perBranch.push_back(bfsDistances(start, graph, flowLookup, backEdges));
        }

        std::string best;
        int bestMaxDist = std::numeric_limits<int>::max();

        for (const auto& [node, d0] : perBranch[0]) {
            bool inAll = true;
            int maxDist = d0;
            for (size_t i = 1; i < perBranch.size(); ++i) {
                auto it = perBranch[i].find(node);
                if (it == perBranch[i].end()) { inAll = false; break; }
                maxDist = std::max(maxDist, it->second);
            }
            if (inAll && maxDist < bestMaxDist) {
                bestMaxDist = maxDist;
                best = node;
            }
        }
        return best;
    }

} // namespace

FlowAnalysis analyzeFlow(const ActivityDiagram& diagram, const ActivityGraph& graph) {
    FlowAnalysis result;
    auto flowLookup = buildFlowLookup(diagram);

    // --- Initial / Final detection ---
    for (const auto& n : diagram.nodes) {
        if (n.kind == ActivityNodeKind::Initial) {
            if (result.initialNodeId.empty()) {
                result.initialNodeId = n.id;
            }
            else {
                std::cerr << "[FlowAnalysis] Multiple InitialNodes found; using '"
                    << result.initialNodeId << "', ignoring '" << n.id << "'\n";
            }
        }
        if (n.kind == ActivityNodeKind::ActivityFinal || n.kind == ActivityNodeKind::FlowFinal) {
            result.finalNodeIds.push_back(n.id);
        }
    }
    if (result.initialNodeId.empty()) {
        std::cerr << "[FlowAnalysis] No InitialNode found\n";
    }
    // Structurally-implied initial/final nodes not modeled as such
    for (const auto& n : diagram.nodes) {
        auto inCount = graph.incoming.count(n.id) ? graph.incoming.at(n.id).size() : 0;
        auto outCount = graph.outgoing.count(n.id) ? graph.outgoing.at(n.id).size() : 0;
        if (inCount == 0 && n.kind != ActivityNodeKind::Initial) {
            std::cerr << "[FlowAnalysis] Node '" << n.id
                << "' has no incoming flows but is not an InitialNode\n";
        }
        if (outCount == 0 && n.kind != ActivityNodeKind::ActivityFinal
            && n.kind != ActivityNodeKind::FlowFinal) {
            std::cerr << "[FlowAnalysis] Node '" << n.id
                << "' has no outgoing flows but is not a FinalNode\n";
        }
    }

    // --- Back-edge detection (DFS white/gray/black over the whole diagram,
    //     not just from initialNodeId, so disconnected components and any
    //     extra sub-loops are still classified correctly) ---
    enum class Color { White, Gray, Black };
    std::unordered_map<std::string, Color> color;
    for (const auto& n : diagram.nodes) color[n.id] = Color::White;

    std::function<void(const std::string&)> dfs = [&](const std::string& u) {
        color[u] = Color::Gray;
        auto it = graph.outgoing.find(u);
        if (it != graph.outgoing.end()) {
            for (const auto& flowId : it->second) {
                const ActivityFlow* f = flowLookup.at(flowId);
                if (color[f->toId] == Color::White) {
                    dfs(f->toId);
                }
                else if (color[f->toId] == Color::Gray) {
                    result.backEdgeFlowIds.insert(flowId);
                }
                // Black target = forward/cross edge; not a cycle, keep as-is.
            }
        }
        color[u] = Color::Black;
        };
    if (!result.initialNodeId.empty()) dfs(result.initialNodeId);
    for (const auto& n : diagram.nodes) {
        if (color[n.id] == Color::White) dfs(n.id);
    }

    // --- Connected components (undirected reachability over all flows) ---
    std::unordered_map<std::string, std::vector<std::string>> undirected;
    for (const auto& n : diagram.nodes) undirected[n.id]; // ensure isolated nodes appear
    for (const auto& f : diagram.flows) {
        undirected[f.fromId].push_back(f.toId);
        undirected[f.toId].push_back(f.fromId);
    }
    std::unordered_set<std::string> visited;
    std::vector<std::vector<std::string>> components;
    for (const auto& n : diagram.nodes) {
        if (visited.count(n.id)) continue;
        std::vector<std::string> component;
        std::queue<std::string> q;
        q.push(n.id);
        visited.insert(n.id);
        while (!q.empty()) {
            std::string u = q.front(); q.pop();
            component.push_back(u);
            for (const auto& v : undirected[u]) {
                if (!visited.count(v)) {
                    visited.insert(v);
                    q.push(v);
                }
            }
        }
        components.push_back(std::move(component));
    }
    // Put the component containing initialNodeId first.
    if (!result.initialNodeId.empty()) {
        std::stable_partition(components.begin(), components.end(),
            [&](const std::vector<std::string>& c) {
                return std::find(c.begin(), c.end(), result.initialNodeId) != c.end();
            });
    }
    result.components = std::move(components);

    // --- Decision/Merge pairing ---
    for (const auto& n : diagram.nodes) {
        if (n.kind != ActivityNodeKind::Decision) continue;
        BranchPairing pairing;
        pairing.splitId = n.id;
        auto it = graph.outgoing.find(n.id);
        if (it != graph.outgoing.end()) {
            for (const auto& flowId : it->second) {
                if (result.backEdgeFlowIds.count(flowId)) continue;
                pairing.branchStartIds.push_back(flowLookup.at(flowId)->toId);
            }
        }
        pairing.joinId = findNearestConvergence(pairing.branchStartIds, graph, flowLookup, result.backEdgeFlowIds);
        result.decisionMergePairs.push_back(std::move(pairing));
    }

    // --- Fork/Join pairing ---
    for (const auto& n : diagram.nodes) {
        if (n.kind != ActivityNodeKind::Fork) continue;
        BranchPairing pairing;
        pairing.splitId = n.id;
        auto it = graph.outgoing.find(n.id);
        if (it != graph.outgoing.end()) {
            for (const auto& flowId : it->second) {
                if (result.backEdgeFlowIds.count(flowId)) continue;
                pairing.branchStartIds.push_back(flowLookup.at(flowId)->toId);
            }
        }
        pairing.joinId = findNearestConvergence(pairing.branchStartIds, graph, flowLookup, result.backEdgeFlowIds);

        if (pairing.joinId.empty()) {
            std::cerr << "[FlowAnalysis] Fork node '" << n.id
                << "' has branches that never reconverge at a Join\n";
            for (const auto& b : pairing.branchStartIds) {
                result.unmatchedForkBranchStartIds.push_back(b);
            }
        }
        else {
            // Per design: every Fork branch is expected to reconverge — verify
            // the convergence point actually is a Join node, not something else.
            const ActivityNode* joinNode = nullptr;
            for (const auto& node : diagram.nodes) {
                if (node.id == pairing.joinId) { joinNode = &node; break; }
            }
            if (joinNode == nullptr || joinNode->kind != ActivityNodeKind::Join) {
                std::cerr << "[FlowAnalysis] Fork node '" << n.id
                    << "' branches converge at '" << pairing.joinId
                    << "' which is not a Join node\n";
            }
        }
        result.forkJoinPairs.push_back(std::move(pairing));
    }

    return result;
}