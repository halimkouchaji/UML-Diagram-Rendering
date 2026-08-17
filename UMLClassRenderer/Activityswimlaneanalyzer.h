#pragma once
#include "ActivityModel.h"
#include "ActivityGraphBuilder.h"
#include "ActivityFlowAnalyzer.h"

// ---------------------------------------------------------------------
// Phase 4 — Swimlane Analysis
//
// Finalizes swimlane membership/order/orientation. Sizing is only
// reserved here (a "slot" per the design doc) — final extents are
// computed in Phase 7 once node positions exist, then grown further in
// Phase 9's collision cleanup pass if needed.
//
// Order rule: if the XMI supplied an explicit orderIndex (any value
// other than the default -1... — see note below), that order is
// respected as-is. Otherwise lanes are ordered by the average Phase 5
// layer of the nodes they contain, so this function's *order* step must
// run after Phase 5, even though membership/orientation are resolved
// immediately. Call analyzeSwimlanes() again after layering to finalize
// order — see finalizeSwimlaneOrder() below.
// ---------------------------------------------------------------------

// Resolves membership + orientation only. Call once, early, right after
// Phase 2/3. Assumes assignImplicitSwimlanes() has already been run so
// every node has a non-empty swimlaneId.
void analyzeSwimlanes(ActivityDiagram& diagram);

// Re-sorts diagram.swimlanes by the average `layer` of their member
// nodes (ascending), and rewrites each lane's orderIndex to match.
// Must be called after Phase 5 (Layer Assignment) has populated
// ActivityNode::layer for every node. Lanes with an explicit XMI-given
// order (hasExplicitOrder == true) are left untouched and other lanes
// are interleaved around them by layer average.
void finalizeSwimlaneOrder(ActivityDiagram& diagram);