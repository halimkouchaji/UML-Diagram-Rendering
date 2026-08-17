#pragma once
#include "ActivityModel.h"
#include "ActivityCoordinateAssigner.h"
#include "ActivityFlowAnalyzer.h"

// ---------------------------------------------------------------------
// Phase 8 — Edge Routing
//
// Fills ActivityFlow::points and ActivityLayoutContext::edgeLabels.
// Uses orthogonal routing, with back-edge loop routing.
// ---------------------------------------------------------------------
void routeActivityEdges(ActivityDiagram& diagram,
    ActivityLayoutContext& ctx,
    const FlowAnalysis& flowAnalysis);