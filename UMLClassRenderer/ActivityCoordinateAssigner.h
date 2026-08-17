#pragma once
#include "ActivityModel.h"
#include <string>
#include <vector>

struct ActivityEdgeLabel {
    std::string flowId;
    std::string text;
    double x = 0.0;
    double y = 0.0;
    double width = 0.0;
    double height = 0.0;
};

struct ActivityLayoutContext {
    double diagramWidth = 0.0;
    double diagramHeight = 0.0;

    double padding = 40.0;
    double swimlanePadding = 36.0;
    double laneHeaderSize = 34.0;
    double layerSpacing = 130.0;
    double nodeSpacing = 110.0;

    std::vector<ActivityEdgeLabel> edgeLabels;
    std::vector<std::string> warnings;
};

// ---------------------------------------------------------------------
// Phase 7 — Coordinate Assignment
//
// Consumes ActivityNode::layer and ActivityNode::orderInLayer from
// Phase 5/6. Mutates node sizes, node coordinates, and swimlane bounds.
// Returns layout-wide bounds/warnings used by Phase 8-10.
// ---------------------------------------------------------------------
ActivityLayoutContext assignActivityCoordinates(ActivityDiagram& diagram);