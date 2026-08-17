#pragma once
#include "Model.h"
#include <string>

// Serializes the final ClassDiagram (positions + routed edges)
// into a JSON file another renderer can load and draw.
void exportToJson(const ClassDiagram& diagram, const std::string& outPath);