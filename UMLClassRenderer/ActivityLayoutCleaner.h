#pragma once
#include "ActivityModel.h"
#include "ActivityCoordinateAssigner.h"
#include "ActivityFlowAnalyzer.h"

// ---------------------------------------------------------------------
// Phase 9 — Collision & Layout Cleanup
//
// Fixes node overlap, grows swimlanes, recomputes diagram bounds,
// and reroutes edges once after movement.
// ---------------------------------------------------------------------
void cleanupActivityLayout(ActivityDiagram& diagram,
    ActivityLayoutContext& ctx,
    const FlowAnalysis& flowAnalysis);