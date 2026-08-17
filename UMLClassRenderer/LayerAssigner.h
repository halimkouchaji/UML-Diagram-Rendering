#pragma once
#pragma once
#include "Model.h"
#include "HierarchyBuilder.h"

// Assigns diagram.classes[i].level based on inheritance depth from roots.
// Root classes get level 0; children get parent's level + 1.
// Classes with no inheritance relationship at all (true orphans,
// not in parentOf or childrenOf) are treated as their own root at level 0.
void assignLayers(ClassDiagram& diagram, const HierarchyInfo& hierarchy);