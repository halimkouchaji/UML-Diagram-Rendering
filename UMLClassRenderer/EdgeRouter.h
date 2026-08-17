#pragma once
#include "Model.h"

// Computes edge.points for every edge in the diagram, based on
// the final x/y/width/height of the classes each edge connects.
void routeEdges(ClassDiagram& diagram);