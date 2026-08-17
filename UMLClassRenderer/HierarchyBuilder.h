#pragma once
#include "Model.h"
#include <string>
#include <vector>
#include <unordered_map>

struct HierarchyInfo {
    // A child can have multiple parents now: one generalization parent
    // AND any number of realized interfaces, coexisting without conflict.
    std::unordered_map<std::string, std::vector<std::string>> parentsOf;
    std::unordered_map<std::string, std::vector<std::string>> childrenOf;
    std::vector<std::string> roots; // classes/interfaces with no parents at all
};

// Walks Generalization AND Realization edges to build a multi-parent
// hierarchy (a class can inherit from one class AND realize interfaces
// at the same time — these no longer compete for a single "parent" slot).
HierarchyInfo buildHierarchy(const ClassDiagram& diagram);