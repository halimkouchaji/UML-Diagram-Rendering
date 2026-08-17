#include <iostream>

#include "XmiParser.h"
#include "HierarchyBuilder.h"
#include "NodeSizer.h"
#include "LayerAssigner.h"
#include "LayerOrderer.h"
#include "CoordinateAssigner.h"
#include "EdgeRouter.h"
#include "OverlapResolver.h"
#include "JsonExporter.h"
#include "SvgRenderer.h"

int main()
{
    // Phase 1: Parse XMI
    ClassDiagram diagram = parseXmiFile("sample.xmi");

    // Phase 2: Build hierarchy
    HierarchyInfo hierarchy = buildHierarchy(diagram);

    // Phase 3+: Layout pipeline
    computeNodeSizes(diagram);
    assignLayers(diagram, hierarchy);
    assignOrder(diagram, hierarchy);
    assignCoordinates(diagram, hierarchy);
    routeEdges(diagram);
    resolveOverlapsAndNormalize(diagram);

    // Export layout to JSON
    exportToJson(diagram, "layout_output.json");

    // Render JSON to SVG
    renderToSvg("layout_output.json", "diagram.svg");

    std::cout << "Done!\n";
    std::cout << "Generated: layout_output.json\n";
    std::cout << "Generated: diagram.svg\n";

    return 0;
}