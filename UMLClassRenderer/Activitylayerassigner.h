#pragma once
#include "ActivityModel.h"
#include "ActivityGraphBuilder.h"
#include "ActivityFlowAnalyzer.h"

// ---------------------------------------------------------------------
// Phase 5 — Layer Assignment
//
// Assigns ActivityNode::layer for every node via longest-path DP over
// the back-edge-free DAG (back edges come from FlowAnalysis::backEdgeFlowIds
// and are excluded here — this is exactly what keeps loops from causing
// infinite/incorrect layer growth; see Phase 3).
//
// Each connected component (FlowAnalysis::components) is laid out
// against its own local layer-0 origin; Phase 7 is responsible for
// translating disconnected components apart spatially.
//
// After the base pass, Fork/Join branches (FlowAnalysis::forkJoinPairs)
// are synchronized: shorter branches get invisible spacer ActivityNodes
// (isSpacer = true) inserted at every layer between the Fork and Join
// that the branch doesn't already occupy, so every branch reserves a
// slot at every layer the parallel group spans. Decision/Merge branches
// are NOT padded this way — per the design doc they're allowed to run
// independent lengths, and the standard longest-path rule already
// places the Merge correctly without help.
// ---------------------------------------------------------------------

void assignLayers(ActivityDiagram& diagram, const ActivityGraph& graph, const FlowAnalysis& flowAnalysis);