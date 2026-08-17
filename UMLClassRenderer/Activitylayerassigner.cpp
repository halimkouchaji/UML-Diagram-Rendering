#include "ActivityLayerAssigner.h"
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <iostream>
#include <algorithm>

namespace {

    std::unordered_map<std::string, const ActivityFlow*> buildFlowLookup(const ActivityDiagram& diagram) {
        std::unordered_map<std::string, const ActivityFlow*> lookup;
        for (const auto& f : diagram.flows) lookup[f.id] = &f;
        return lookup;
    }

    // Longest-path layering via Kahn's algorithm, restricted to one connected
    // component and to non-back-edge flows.
    void assignLayersForComponent(
        const std::vector<std::string>& componentNodeIds,
        ActivityDiagram& diagram,
        const ActivityGraph& graph,
        const std::unordered_map<std::string, const ActivityFlow*>& flowLookup,
        const std::unordered_set<std::string>& backEdges)
    {
        std::unordered_set<std::string> inComponent(componentNodeIds.begin(), componentNodeIds.end());

        std::unordered_map<std::string, int> indeg;
        for (const auto& id : componentNodeIds) indeg[id] = 0;
        for (const auto& id : componentNodeIds) {
            auto it = graph.outgoing.find(id);
            if (it == graph.outgoing.end()) continue;
            for (const auto& flowId : it->second) {
                if (backEdges.count(flowId)) continue;
                const ActivityFlow* f = flowLookup.at(flowId);
                if (inComponent.count(f->toId)) indeg[f->toId]++;
            }
        }

        std::queue<std::string> q;
        for (const auto& id : componentNodeIds) {
            if (indeg[id] == 0) q.push(id);
        }

        std::unordered_map<std::string, int> layer;
        for (const auto& id : componentNodeIds) layer[id] = 0;

        int processed = 0;
        while (!q.empty()) {
            std::string u = q.front(); q.pop();
            processed++;
            auto it = graph.outgoing.find(u);
            if (it != graph.outgoing.end()) {
                for (const auto& flowId : it->second) {
                    if (backEdges.count(flowId)) continue;
                    const ActivityFlow* f = flowLookup.at(flowId);
                    if (!inComponent.count(f->toId)) continue;
                    layer[f->toId] = std::max(layer[f->toId], layer[u] + 1);
                    if (--indeg[f->toId] == 0) q.push(f->toId);
                }
            }
        }

        if (processed != static_cast<int>(componentNodeIds.size())) {
            std::cerr << "[LayerAssigner] Component layering only processed "
                << processed << "/" << componentNodeIds.size()
                << " nodes — a cycle may remain after back-edge removal\n";
        }

        for (const auto& id : componentNodeIds) {
            ActivityNode* n = diagram.findNodeById(id);
            if (n) n->layer = layer[id];
        }
    }

} // namespace

void assignLayers(ActivityDiagram& diagram, const ActivityGraph& graph, const FlowAnalysis& flowAnalysis) {
    auto flowLookup = buildFlowLookup(diagram);

    for (const auto& component : flowAnalysis.components) {
        assignLayersForComponent(component, diagram, graph, flowLookup, flowAnalysis.backEdgeFlowIds);
    }

    // --- Fork/Join spacer sync ---
    // Note: this appends new spacer ActivityNodes to diagram.nodes while
    // iterating flowAnalysis (a separate, already-computed structure), so
    // it's safe — flowAnalysis.forkJoinPairs doesn't change underfoot.
    int spacerCounter = 0;
    for (const auto& pairing : flowAnalysis.forkJoinPairs) {
        if (pairing.joinId.empty()) continue; // unmatched — already warned in Phase 3

        ActivityNode* forkNode = diagram.findNodeById(pairing.splitId);
        ActivityNode* joinNode = diagram.findNodeById(pairing.joinId);
        if (!forkNode || !joinNode) continue;

        for (size_t branchIdx = 0; branchIdx < pairing.branchStartIds.size(); ++branchIdx) {
            const std::string& branchStart = pairing.branchStartIds[branchIdx];

            // This branch's "territory": nodes reachable from branchStart
            // without passing through the Join itself.
            std::unordered_set<std::string> territory;
            std::queue<std::string> q;
            q.push(branchStart);
            territory.insert(branchStart);
            while (!q.empty()) {
                std::string u = q.front(); q.pop();
                auto it = graph.outgoing.find(u);
                if (it == graph.outgoing.end()) continue;
                for (const auto& flowId : it->second) {
                    if (flowAnalysis.backEdgeFlowIds.count(flowId)) continue;
                    const ActivityFlow* f = flowLookup.at(flowId);
                    if (f->toId == pairing.joinId) continue; // stop at the join
                    if (territory.count(f->toId)) continue;
                    ActivityNode* target = diagram.findNodeById(f->toId);
                    if (target && target->layer < joinNode->layer) {
                        territory.insert(f->toId);
                        q.push(f->toId);
                    }
                }
            }

            std::unordered_set<int> occupiedLayers;
            std::string lastRealSwimlane;
            for (const auto& id : territory) {
                ActivityNode* n = diagram.findNodeById(id);
                if (n) {
                    occupiedLayers.insert(n->layer);
                    lastRealSwimlane = n->swimlaneId;
                }
            }

            for (int L = forkNode->layer + 1; L < joinNode->layer; ++L) {
                if (occupiedLayers.count(L)) continue;

                ActivityNode spacer;
                spacer.id = "__spacer__" + pairing.splitId + "__" + std::to_string(branchIdx)
                    + "__" + std::to_string(spacerCounter++);
                spacer.kind = ActivityNodeKind::Action; // shape irrelevant — Phase 10 skips isSpacer nodes
                spacer.isSpacer = true;
                spacer.layer = L;
                spacer.swimlaneId = !lastRealSwimlane.empty() ? lastRealSwimlane : forkNode->swimlaneId;
                diagram.nodes.push_back(spacer);
            }
        }
    }
}