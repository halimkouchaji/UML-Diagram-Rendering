#pragma once
#include "Model.h"
#include "HierarchyBuilder.h"

// Computes final x, y pixel coordinates for every class box,
// based on level (row), orderInLevel (column), and box size.
void assignCoordinates(ClassDiagram& diagram, const HierarchyInfo& hierarchy);