#pragma once
#include "Model.h"
#include "HierarchyBuilder.h"

// Assigns diagram.classes[i].orderInLevel — the left-to-right
// position of each class within its level. Uses a barycenter
// heuristic based on parent position to reduce edge crossings.
void assignOrder(ClassDiagram& diagram, const HierarchyInfo& hierarchy);