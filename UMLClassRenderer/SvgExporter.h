#pragma once
#include "Model.h"
#include <string>

// Renders the final laid-out ClassDiagram to an SVG file, including
// proper UML notation per edge type (triangles, diamonds, dashed lines).
void exportToSvg(const ClassDiagram& diagram, const std::string& outPath);