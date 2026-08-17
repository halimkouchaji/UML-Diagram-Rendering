#include "JsonExporter.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

namespace {
    std::string visibilityToString(Visibility v) {
        switch (v) {
        case Visibility::Public:    return "public";
        case Visibility::Private:   return "private";
        case Visibility::Protected: return "protected";
        case Visibility::Package:   return "package";
        }
        return "private";
    }

    std::string edgeTypeToString(EdgeType t) {
        switch (t) {
        case EdgeType::Generalization: return "generalization";
        case EdgeType::Association:    return "association";
        case EdgeType::Aggregation:     return "aggregation";
        case EdgeType::Composition:     return "composition";
        case EdgeType::Dependency:      return "dependency";
        case EdgeType::Realization:     return "realization";
        }
        return "association";
    }
}

void exportToJson(const ClassDiagram& diagram, const std::string& outPath) {
    json j;
    j["classes"] = json::array();
    j["edges"] = json::array();

    for (const auto& cls : diagram.classes) {
        json c;
        c["id"] = cls.id;
        c["name"] = cls.name;
        c["x"] = cls.x;
        c["y"] = cls.y;
        c["width"] = cls.width;
        c["height"] = cls.height;
        c["level"] = cls.level;

        c["attributes"] = json::array();
        for (const auto& a : cls.attributes) {
            c["attributes"].push_back({
                {"name", a.name},
                {"type", a.type},
                {"visibility", visibilityToString(a.visibility)}
                });
        }

        c["operations"] = json::array();
        for (const auto& o : cls.operations) {
            c["operations"].push_back({
                {"name", o.name},
                {"returnType", o.returnType},
                {"visibility", visibilityToString(o.visibility)}
                });
        }

        j["classes"].push_back(c);
    }

    for (const auto& e : diagram.edges) {
        json edgeJson;
        edgeJson["id"] = e.id;
        edgeJson["type"] = edgeTypeToString(e.type);
        edgeJson["from"] = e.fromId;
        edgeJson["to"] = e.toId;

        edgeJson["points"] = json::array();
        for (const auto& p : e.points) {
            edgeJson["points"].push_back({ {"x", p.x}, {"y", p.y} });
        }

        j["edges"].push_back(edgeJson);
    }

    std::ofstream file(outPath);
    if (!file) {
        std::cerr << "Failed to open output file: " << outPath << std::endl;
        return;
    }
    file << j.dump(2); // pretty-print with 2-space indent
    std::cout << "Wrote layout to " << outPath << std::endl;
}