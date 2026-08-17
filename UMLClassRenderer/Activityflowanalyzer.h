#pragma once
#include "ActivityModel.h"
#include "ActivityGraphBuilder.h"
#include <string>
#include <vector>
#include <unordered_set>

// ---------------------------------------------------------------------
// Phase 3 — Flow Analysis
//
// Classifies the graph before any layout math happens. Nothing here
// assigns coordinates or layers — it produces facts that Phase 5/6/8
// consume:
//   - which flows are back edges (excluded from layering, routed
//     specially)
//   - Decision/Merge and Fork/Join pairings (used for layer sync and
//     branch-order locking)
//   - disconnected components (laid out independently in Phase 7)
// ---------------------------------------------------------------------

struct BranchPairing {
    std::string splitId;                    // Decision or Fork node id
    std::string joinId;                     // matching Merge or Join node id (empty if unmatched)
    std::vector<std::string> branchStartIds; // first node id of each outgoing branch, in flow order
};

struct FlowAnalysis {
    std::string initialNodeId;                  // empty if none/ambiguous found
    std::vector<std::string> finalNodeIds;

    std::unordered_set<std::string> backEdgeFlowIds; // flow ids classified as loop-back

    std::vector<BranchPairing> decisionMergePairs;   // unmatched -> joinId == ""
    std::vector<BranchPairing> forkJoinPairs;        // unmatched branch flagged separately below

    std::vector<std::string> unmatchedForkBranchStartIds; // validation warning list

    // Connected components, expressed as node-id sets. Index 0 is always
    // the component containing initialNodeId (if one was found); the
    // rest are disconnected components in discovery order.
    std::vector<std::vector<std::string>> components;
};

FlowAnalysis analyzeFlow(const ActivityDiagram& diagram, const ActivityGraph& graph);