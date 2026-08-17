#pragma once
#include "Model.h"

// Computes width/height for every class box based on its name,
// attribute list, and operation list. Mutates diagram.classes in place.
void computeNodeSizes(ClassDiagram& diagram);