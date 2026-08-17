#pragma once
#include "ActivityModel.h"
#include <string>
#include <vector>
#include <unordered_map>

// ---------------------------------------------------------------------
// Phase 2 — Graph Construction
//
// Builds adjacency lists over the flat ActivityDiagram model, resolves
// implicit swimlane membership, and validates structural consistency.
// Nothing here computes layout — this phase only makes the graph
// queryable for Phase 3 (Flow Analysis) onward.
// ---------------------------------------------------------------------

struct ActivityGraph {
    // node id -> ids of ControlFlow/ObjectFlow entries leaving/entering it
    std::unordered_map<std::string, std::vector<std::string>> outgoing;
    std::unordered_map<std::string, std::vector<std::string>> incoming;
};

// Assigns every node with an empty swimlaneId to an implicit "unassigned"
// lane, creating that Swimlane entry on the diagram if it doesn't already
// exist. Mutates diagram in place so every node has a resolvable
// swimlaneId afterward — downstream phases never special-case "no lane".
void assignImplicitSwimlanes(ActivityDiagram& diagram);

// Builds the outgoing/incoming adjacency lists from diagram.flows.
ActivityGraph buildActivityGraph(const ActivityDiagram& diagram);

// Structural validation (see Phase 2 rules in the design doc). Problems
// are reported via std::cerr as warnings and do not throw — callers can
// inspect the returned bool to decide whether to abort. This mirrors
// the non-fatal, print-and-continue style already used in the Class
// Diagram pipeline (e.g. JsonExporter's file-open check).
//
// Checks performed:
//   - every flow's fromId/toId resolves to an existing node
//   - every DecisionNode has >= 2 outgoing flows
//   - every MergeNode has >= 2 incoming flows
//   - every ForkNode has >= 2 outgoing flows
//   - every JoinNode has >= 2 incoming flows
//   - at least one InitialNode exists
//
// Returns true if no problems were found.
bool validateActivityGraph(const ActivityDiagram& diagram, const ActivityGraph& graph);