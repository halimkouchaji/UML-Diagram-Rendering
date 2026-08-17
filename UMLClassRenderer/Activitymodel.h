#pragma once
#include "Model.h"   // reuses the existing Point{x,y} struct
#include <string>
#include <vector>

// ---------------------------------------------------------------------
// Phase 1 — Data Model
//
// Mirrors the style of the Class Diagram model (single flat structs per
// element, not deep polymorphism) so downstream phases (Graph Construction,
// Flow Analysis, Layer Assignment, etc.) can operate over a single
// std::vector<ActivityNode> without type-switching except where node shape
// actually matters (sizing / drawing).
// ---------------------------------------------------------------------

enum class ActivityNodeKind {
    Initial,
    Action,        // "Activity" node in the spec
    Decision,
    Merge,
    Fork,
    Join,
    ActivityFinal, // ringed circle
    FlowFinal,     // circle-with-X
    Object         // optional
};

struct ActivityNode {
    std::string id;
    std::string name;                  // empty for Initial/Final/Fork/Join
    ActivityNodeKind kind = ActivityNodeKind::Action;

    // Original XMI position, used only as an ordering hint (Phase 6) —
    // never copied directly into final layout coordinates.
    double xmiX = 0.0;
    double xmiY = 0.0;
    bool hasXmiCoords = false;

    std::string swimlaneId;            // empty = implicit "unassigned" lane

    // Fork/Join only: true = horizontal bar, false = vertical bar
    bool horizontalBar = false;

    // --- filled in by later phases, not by the parser ---
    int layer = -1;                    // Phase 5
    int orderInLayer = -1;             // Phase 6
    bool isSpacer = false;             // Phase 5 — invisible Fork/Join sync padding
    double x = 0.0, y = 0.0;           // Phase 7
    double width = 0.0, height = 0.0;  // Phase 7 (sizing pass, analogous to NodeSizer)
};

struct Swimlane {
    std::string id;
    std::string name;
    int orderIndex = 0;                // Phase 4 (finalized after Phase 5)
    bool horizontal = false;           // default vertical column per design doc

    // --- filled in by Phase 4 (slot) / Phase 7 (final extent) ---
    double x = 0.0, y = 0.0;
    double width = 0.0, height = 0.0;
};

enum class ActivityFlowType {
    Control,
    Object   // optional
};

struct ActivityFlow {
    std::string id;
    ActivityFlowType type = ActivityFlowType::Control;
    std::string fromId;
    std::string toId;

    std::string guardLabel;            // e.g. "[yes]" — Control Flow only, optional
    std::string objectLabel;           // Object Flow only, optional

    // --- filled in by later phases ---
    bool isBackEdge = false;           // Phase 3 — excluded from layering, routed specially (Phase 8)
    std::vector<Point> points;         // Phase 8 — final routed path
};

struct ActivityDiagram {
    std::vector<ActivityNode> nodes;
    std::vector<Swimlane> swimlanes;
    std::vector<ActivityFlow> flows;

    // Helpers: mirror ClassDiagram::findById — Phase 2 onward resolves
    // from/to and swimlane references by id constantly.
    ActivityNode* findNodeById(const std::string& id) {
        for (auto& n : nodes) {
            if (n.id == id) return &n;
        }
        return nullptr;
    }

    Swimlane* findSwimlaneById(const std::string& id) {
        for (auto& s : swimlanes) {
            if (s.id == id) return &s;
        }
        return nullptr;
    }
};