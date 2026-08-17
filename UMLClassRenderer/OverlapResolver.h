#pragma once
#pragma once
#include "Model.h"

// Final cleanup pass: (1) resolves any horizontal overlap between
// sibling boxes in the same level, (2) normalizes the whole diagram
// so the top-left-most point is at (0,0).
void resolveOverlapsAndNormalize(ClassDiagram& diagram);