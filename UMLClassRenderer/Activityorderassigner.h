#pragma once
#include "ActivityModel.h"
#include "ActivityGraphBuilder.h"
#include "ActivityFlowAnalyzer.h"

// ---------------------------------------------------------------------
// Phase 6 — Ordering Within Layers
//
// Sets ActivityNode::orderInLayer for every node (including spacers) to
// its position within its OWN SWIMLANE's slice of its layer (0-based).
// This is exactly what Phase 7 consumes directly for horizontal
// placement (x = swimlane_x_offset + orderInLayer * spacing).
//
// Approach: Sugiyama-style barycenter passes (alternating top-down /
// bottom-up sweeps) minimize edge crossings, with two hard constraints:
//   - a node only ever reorders relative to other nodes in the SAME
//     swimlane at its layer — swimlane membership never changes which
//     column a node lands in, only its position within that column
//   - Fork/Join branches keep a locked relative order (by branch index)
//     across every layer they span, so parallel branches read as
//     straight lanes instead of crossing over each other. Decision/Merge
//     branches are NOT locked this way — ordinary crossing-minimization
//     applies to them, per the design doc.
//
// Known simplification: barycenter neighbor lookup only considers flows
// connecting immediately adjacent layers. Flows skipping layers
// shouldn't normally occur outside Fork/Join spans (which are already
// backed by per-layer spacer nodes from Phase 5); if one does occur, it
// simply doesn't contribute to the sweep step for the layer it skips —
// this affects crossing-minimization quality only, not correctness of
// final coordinates.
// ---------------------------------------------------------------------

void orderWithinLayers(ActivityDiagram& diagram, const ActivityGraph& graph, const FlowAnalysis& flowAnalysis);