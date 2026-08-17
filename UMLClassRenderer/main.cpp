#include <iostream>
#include <filesystem>
#include <vector>
#include <string>

#include "model.h"
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

// Activity Diagram pipeline
#include "ActivityModel.h"
#include "ActivityXmiParser.h"
#include "ActivityGraphBuilder.h"
#include "ActivityFlowAnalyzer.h"
#include "ActivitySwimlaneAnalyzer.h"
#include "ActivityLayerAssigner.h"
#include "ActivityOrderAssigner.h"
#include "ActivityCoordinateAssigner.h"
#include "ActivityEdgeRouter.h"
#include "ActivityLayoutCleaner.h"
#include "ActivityJsonExporter.h"
#include "ActivitySvgRenderer.h"

namespace fs = std::filesystem;

std::vector<fs::path> findXmiFiles(const std::string& folder)
{
    std::vector<fs::path> files;

    if (!fs::exists(folder))
    {
        return files;
    }

    for (const auto& entry : fs::directory_iterator(folder))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".xmi")
        {
            files.push_back(entry.path());
        }
    }

    return files;
}

bool askOverwriteIfNeeded(const std::string& jsonPath, const std::string& svgPath)
{
    if (fs::exists(jsonPath) || fs::exists(svgPath))
    {
        char answer;

        std::cout << "\nOutput files already exist.\n";
        std::cout << "Overwrite them? (Y/N): ";
        std::cin >> answer;

        if (answer != 'Y' && answer != 'y')
        {
            std::cout << "\nOperation cancelled.\n";
            return false;
        }
    }

    return true;
}

int selectFile(const std::vector<fs::path>& files)
{
    std::cout << "Available diagrams:\n\n";

    for (size_t i = 0; i < files.size(); ++i)
    {
        std::cout << i + 1 << ". " << files[i].filename().string() << '\n';
    }

    std::cout << "\nSelect a diagram (1-" << files.size() << "): ";

    int choice;
    std::cin >> choice;

    if (std::cin.fail() ||
        choice < 1 ||
        choice > static_cast<int>(files.size()))
    {
        std::cout << "\nInvalid selection.\n";
        return -1;
    }

    return choice - 1;
}

void renderClassDiagram()
{
    const std::string inputFolder = "Class Diagram Input";
    const std::string jsonFolder = "Class Diagram Json";
    const std::string svgFolder = "Class Diagram Svg";

    fs::create_directory(jsonFolder);
    fs::create_directory(svgFolder);

    std::vector<fs::path> xmiFiles = findXmiFiles(inputFolder);

    if (xmiFiles.empty())
    {
        std::cout << "No XMI files found in the " << inputFolder << " folder.\n";
        return;
    }

    int index = selectFile(xmiFiles);
    if (index < 0) return;

    fs::path selected = xmiFiles[index];

    std::string inputPath = selected.string();
    std::string baseName = selected.stem().string();

    std::string jsonPath = jsonFolder + "/" + baseName + ".json";
    std::string svgPath = svgFolder + "/" + baseName + ".svg";

    if (!askOverwriteIfNeeded(jsonPath, svgPath))
    {
        return;
    }

    std::cout << "\nRendering Class Diagram: " << selected.filename().string() << "...\n\n";

    ClassDiagram diagram = parseXmiFile(inputPath);

    HierarchyInfo hierarchy = buildHierarchy(diagram);

    computeNodeSizes(diagram);
    assignLayers(diagram, hierarchy);
    assignOrder(diagram, hierarchy);
    assignCoordinates(diagram, hierarchy);
    routeEdges(diagram);
    resolveOverlapsAndNormalize(diagram);

    exportToJson(diagram, jsonPath);
    renderToSvg(jsonPath, svgPath);

    std::cout << "\nCompleted successfully.\n";
    std::cout << "JSON : " << jsonPath << '\n';
    std::cout << "SVG  : " << svgPath << '\n';
}

void renderActivityDiagram()
{
    const std::string inputFolder = "Activity Input";
    const std::string jsonFolder = "Activity Json";
    const std::string svgFolder = "Activity svg";

    fs::create_directory(jsonFolder);
    fs::create_directory(svgFolder);

    std::vector<fs::path> xmiFiles = findXmiFiles(inputFolder);

    if (xmiFiles.empty())
    {
        std::cout << "No XMI files found in the " << inputFolder << " folder.\n";
        return;
    }

    int index = selectFile(xmiFiles);
    if (index < 0) return;

    fs::path selected = xmiFiles[index];

    std::string inputPath = selected.string();
    std::string baseName = selected.stem().string();

    std::string jsonPath = jsonFolder + "/" + baseName + ".json";
    std::string svgPath = svgFolder + "/" + baseName + ".svg";

    if (!askOverwriteIfNeeded(jsonPath, svgPath))
    {
        return;
    }

    std::cout << "\nRendering Activity Diagram: " << selected.filename().string() << "...\n\n";

    ActivityDiagram diagram = parseActivityXmiFile(inputPath);

    assignImplicitSwimlanes(diagram);

    ActivityGraph graph = buildActivityGraph(diagram);
    validateActivityGraph(diagram, graph);

    FlowAnalysis flowAnalysis = analyzeFlow(diagram, graph);

    analyzeSwimlanes(diagram);

    assignLayers(diagram, graph, flowAnalysis);

    finalizeSwimlaneOrder(diagram);

    orderWithinLayers(diagram, graph, flowAnalysis);

    ActivityLayoutContext layout = assignActivityCoordinates(diagram);

    routeActivityEdges(diagram, layout, flowAnalysis);

    cleanupActivityLayout(diagram, layout, flowAnalysis);

    exportActivityToJson(diagram, layout, jsonPath);

    renderActivitySvg(diagram, layout, svgPath);

    std::cout << "\nCompleted successfully.\n";
    std::cout << "JSON : " << jsonPath << '\n';
    std::cout << "SVG  : " << svgPath << '\n';
}

int main()
{
    std::cout << "=====================================\n";
    std::cout << "       UML Layout Renderer\n";
    std::cout << "=====================================\n\n";

    std::cout << "Select diagram type:\n\n";
    std::cout << "1. Class Diagram\n";
    std::cout << "2. Activity Diagram\n";

    std::cout << "\nChoice: ";

    int diagramType;
    std::cin >> diagramType;

    if (std::cin.fail())
    {
        std::cout << "\nInvalid selection.\n";
        return 0;
    }

    std::cout << '\n';

    switch (diagramType)
    {
    case 1:
        renderClassDiagram();
        break;

    case 2:
        renderActivityDiagram();
        break;

    default:
        std::cout << "Invalid selection.\n";
        break;
    }

    std::cout << "\n=====================================\n";
    return 0;
}