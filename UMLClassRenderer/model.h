#pragma once
#include <string>
#include <vector>

// ---------- Basic building blocks ----------

struct Point {
    double x = 0.0;
    double y = 0.0;
};

enum class Visibility {
    Public,
    Private,
    Protected,
    Package
};

struct Attribute {
    std::string name;
    std::string type;
    Visibility visibility = Visibility::Public;
};

struct Operation {
    std::string name;
    std::string returnType;
    Visibility visibility = Visibility::Public;
};

// ---------- Class node ----------

struct ClassNode {
    std::string id;              // xmi:id from the source file
    std::string name;
    std::vector<Attribute> attributes;
    std::vector<Operation> operations;
    bool isInterface = false;   
    // Filled in later by layout phases
    int level = -1;              // Phase 4: inheritance depth
    int orderInLevel = -1;       // Phase 5: left-to-right position
    double x = 0.0;              // Phase 6
    double y = 0.0;              // Phase 6
    double width = 0.0;          // Phase 3
    double height = 0.0;         // Phase 3
};

// ---------- Edges ----------

enum class EdgeType {
    Generalization,   // inheritance
    Association,
    Aggregation,
    Composition,
    Dependency,
    Realization
};

struct Edge {
    std::string id;
    EdgeType type = EdgeType::Association;
    std::string fromId;          // source class id
    std::string toId;            // target class id

    // Filled in later by Phase 7 (edge routing)
    std::vector<Point> points;
};

// ---------- The whole diagram, passed between phases ----------

struct ClassDiagram {
    std::vector<ClassNode> classes;
    std::vector<Edge> edges;

    // Helper: find a class by its id (used constantly once we start
    // resolving generalization/association references from XMI)
    ClassNode* findById(const std::string& id) {
        for (auto& c : classes) {
            if (c.id == id) return &c;
        }
        return nullptr;
    }
};