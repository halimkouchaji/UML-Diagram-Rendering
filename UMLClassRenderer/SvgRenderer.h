#pragma once
#include <string>

// Reads a layout JSON file (produced by JsonExporter) and renders
// a UML class diagram as an SVG file.
void renderToSvg(const std::string& jsonPath, const std::string& svgPath);