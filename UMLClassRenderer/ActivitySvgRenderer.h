#pragma once
#include "ActivityModel.h"
#include "ActivityCoordinateAssigner.h"
#include <string>

// ---------------------------------------------------------------------
// Phase 10 — SVG Rendering
//
// Rendering-only phase. It does not assign coordinates or reroute edges.
// ---------------------------------------------------------------------
bool renderActivitySvg(const ActivityDiagram& diagram,
    const ActivityLayoutContext& ctx,
    const std::string& outputPath);