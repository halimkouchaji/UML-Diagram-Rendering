#include "ActivityOrderAssigner.h"
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <queue>
#include <algorithm>

void orderWithinLayers(ActivityDiagram& diagram, const ActivityGraph& /*graph*/, const FlowAnalysis& flowAnalysis) {
    // O(1) node lookup by id.
    std::unordered_map<std::string, size_t> idxOf;
    for (size_t i = 0; i < diagram.nodes.size(); ++i) idxOf[diagram.nodes[i].id] = i;
    auto nodeAt = [&](const std::string& id) -> ActivityNode& { return diagram.nodes[idxOf.at(id)]; };

    // Group node ids by layer.
    std::map<int, std::vector<std::string>> layerOrder;
    int maxLayer = -1;
    for (auto& n : diagram.nodes) {
        layerOrder[n.layer].push_back(n.id);
        maxLayer = std::max(maxLayer, n.layer);
    }

    std::unordered_map<std::string, int> laneOrder;
    for (auto& lane : diagram.swimlanes) laneOrder[lane.id] = lane.orderIndex;
    auto laneOf = [&](const std::string& id) {
        auto it = laneOrder.find(nodeAt(id).swimlaneId);
        return it != laneOrder.end() ? it->second : 0;
        };

    // Initial order per layer: by (lane order, xmi hint if present, else
    // insertion/discovery order — preserved via stable_sort).
    for (auto& [layer, ids] : layerOrder) {
        std::stable_sort(ids.begin(), ids.end(), [&](const std::string& a, const std::string& b) {
            int laneA = laneOf(a), laneB = laneOf(b);
            if (laneA != laneB) return laneA < laneB;
            ActivityNode& na = nodeAt(a); ActivityNode& nb = nodeAt(b);
            double xa = na.hasXmiCoords ? na.xmiX : 1e18;
            double xb = nb.hasXmiCoords ? nb.xmiX : 1e18;
            return xa < xb;
            });
    }

    auto positionMapFor = [&](int layer) {
        std::unordered_map<std::string, int> pos;
        auto it = layerOrder.find(layer);
        if (it != layerOrder.end()) {
            for (size_t i = 0; i < it->second.size(); ++i) pos[it->second[i]] = static_cast<int>(i);
        }
        return pos;
        };

    // Simple id-to-id adjacency (flow ids don't matter here).
    struct Adj { std::vector<std::string> preds, succs; };
    std::unordered_map<std::string, Adj> adj;
    for (auto& f : diagram.flows) {
        adj[f.toId].preds.push_back(f.fromId);
        adj[f.fromId].succs.push_back(f.toId);
    }

    // Reorders `ids` in place: partitions into swimlane blocks (preserving
    // lane order), then stable-sorts each block by barycenter value —
    // this is what enforces the "never leave your lane" hard constraint.
    auto reorderByBarycenter = [&](std::vector<std::string>& ids, const std::unordered_map<std::string, double>& bary) {
        std::map<int, std::vector<std::string>> byLane;
        for (auto& id : ids) byLane[laneOf(id)].push_back(id);
        for (auto& [lane, laneIds] : byLane) {
            std::stable_sort(laneIds.begin(), laneIds.end(), [&](const std::string& a, const std::string& b) {
                return bary.at(a) < bary.at(b);
                });
        }
        ids.clear();
        for (auto& [lane, laneIds] : byLane) {
            for (auto& id : laneIds) ids.push_back(id);
        }
        };

    auto barycenterPass = [&](bool topDown) {
        auto layers = std::vector<int>{};
        for (auto& [L, ids] : layerOrder) layers.push_back(L);
        if (!topDown) std::reverse(layers.begin(), layers.end());

        for (int L : layers) {
            int refLayer = topDown ? L - 1 : L + 1;
            auto refPos = positionMapFor(refLayer);
            auto& ids = layerOrder[L];

            std::unordered_map<std::string, int> currentPos;
            for (size_t i = 0; i < ids.size(); ++i) currentPos[ids[i]] = static_cast<int>(i);

            std::unordered_map<std::string, double> bary;
            for (auto& id : ids) {
                auto& neighbors = topDown ? adj[id].preds : adj[id].succs;
                double sum = 0.0; int cnt = 0;
                for (auto& nb : neighbors) {
                    auto it = refPos.find(nb);
                    if (it != refPos.end()) { sum += it->second; cnt++; }
                }
                // No positioned neighbor in the adjacent layer -> keep this
                // node's current relative position rather than collapsing
                // it to 0 (which would wrongly drag it to the front).
                bary[id] = cnt > 0 ? sum / cnt : static_cast<double>(currentPos[id]);
            }
            reorderByBarycenter(ids, bary);
        }
        };

    // Alternating sweeps: down, up, down, up.
    for (int i = 0; i < 4; ++i) barycenterPass(i % 2 == 0);

    // --- Fork/Join branch order lock ---
    for (auto& pairing : flowAnalysis.forkJoinPairs) {
        if (pairing.joinId.empty()) continue;
        if (!idxOf.count(pairing.splitId) || !idxOf.count(pairing.joinId)) continue;
        ActivityNode& joinNode = nodeAt(pairing.joinId);

        // layer -> [(branchIndex, nodeId), ...] — recomputed via the same
        // territory-BFS approach as Phase 5, since branch membership
        // isn't stored on the node itself.
        std::map<int, std::vector<std::pair<size_t, std::string>>> layerBranchNodes;

        for (size_t branchIdx = 0; branchIdx < pairing.branchStartIds.size(); ++branchIdx) {
            std::unordered_set<std::string> territory;
            std::queue<std::string> q;
            const std::string& start = pairing.branchStartIds[branchIdx];
            if (!idxOf.count(start)) continue;
            q.push(start);
            territory.insert(start);
            while (!q.empty()) {
                std::string u = q.front(); q.pop();
                for (auto& succ : adj[u].succs) {
                    if (succ == pairing.joinId || territory.count(succ) || !idxOf.count(succ)) continue;
                    if (nodeAt(succ).layer < joinNode.layer) {
                        territory.insert(succ);
                        q.push(succ);
                    }
                }
            }
            for (auto& id : territory) {
                layerBranchNodes[nodeAt(id).layer].push_back({ branchIdx, id });
            }
        }

        for (auto& [L, branchNodes] : layerBranchNodes) {
            if (branchNodes.size() < 2) continue; // nothing to lock at this layer
            auto& ids = layerOrder[L];

            std::vector<int> positions;
            for (auto& [bidx, nid] : branchNodes) {
                auto it = std::find(ids.begin(), ids.end(), nid);
                if (it != ids.end()) positions.push_back(static_cast<int>(it - ids.begin()));
            }
            if (positions.empty()) continue;
            std::sort(positions.begin(), positions.end());

            std::vector<std::pair<size_t, std::string>> sorted = branchNodes;
            std::sort(sorted.begin(), sorted.end(), [](auto& a, auto& b) { return a.first < b.first; });

            for (size_t k = 0; k < positions.size() && k < sorted.size(); ++k) {
                ids[positions[k]] = sorted[k].second;
            }
        }
    }

    // --- Write back orderInLayer as the within-lane index Phase 7 needs ---
    for (auto& [layer, ids] : layerOrder) {
        std::unordered_map<int, int> nextIndexPerLane;
        for (auto& id : ids) {
            int lane = laneOf(id);
            nodeAt(id).orderInLayer = nextIndexPerLane[lane]++;
        }
    }
}