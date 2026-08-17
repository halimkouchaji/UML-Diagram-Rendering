#include "ActivityJsonExporter.h"
#include <fstream>
#include <iostream>

namespace {

    std::string esc(const std::string& s) {
        std::string out;
        for (char c : s) {
            switch (c) {
            case '\\': out += "\\\\"; break;
            case '"':  out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': break;
            case '\t': out += "\\t"; break;
            default:   out += c; break;
            }
        }
        return out;
    }

    std::string nodeKindToString(ActivityNodeKind kind) {
        switch (kind) {
        case ActivityNodeKind::Initial: return "Initial";
        case ActivityNodeKind::Action: return "Action";
        case ActivityNodeKind::Decision: return "Decision";
        case ActivityNodeKind::Merge: return "Merge";
        case ActivityNodeKind::Fork: return "Fork";
        case ActivityNodeKind::Join: return "Join";
        case ActivityNodeKind::ActivityFinal: return "ActivityFinal";
        case ActivityNodeKind::FlowFinal: return "FlowFinal";
        case ActivityNodeKind::Object: return "Object";
        default: return "Unknown";
        }
    }

    std::string flowTypeToString(ActivityFlowType type) {
        return type == ActivityFlowType::Object ? "Object" : "Control";
    }

} // namespace

void exportActivityToJson(const ActivityDiagram& diagram,
    const ActivityLayoutContext& layout,
    const std::string& outputPath) {
    std::ofstream out(outputPath);
    if (!out.is_open()) {
        std::cerr << "[ActivityJsonExporter] Cannot write file: " << outputPath << "\n";
        return;
    }

    out << "{\n";
    out << "  \"diagramType\": \"ActivityDiagram\",\n";
    out << "  \"width\": " << layout.diagramWidth << ",\n";
    out << "  \"height\": " << layout.diagramHeight << ",\n";

    out << "  \"swimlanes\": [\n";
    for (size_t i = 0; i < diagram.swimlanes.size(); ++i) {
        const auto& s = diagram.swimlanes[i];
        out << "    {"
            << "\"id\":\"" << esc(s.id) << "\","
            << "\"name\":\"" << esc(s.name) << "\","
            << "\"x\":" << s.x << ","
            << "\"y\":" << s.y << ","
            << "\"width\":" << s.width << ","
            << "\"height\":" << s.height
            << "}";
        if (i + 1 < diagram.swimlanes.size()) out << ",";
        out << "\n";
    }
    out << "  ],\n";

    out << "  \"nodes\": [\n";
    bool firstNode = true;
    for (const auto& n : diagram.nodes) {
        if (n.isSpacer) continue;

        if (!firstNode) out << ",\n";
        firstNode = false;

        out << "    {"
            << "\"id\":\"" << esc(n.id) << "\","
            << "\"name\":\"" << esc(n.name) << "\","
            << "\"type\":\"" << nodeKindToString(n.kind) << "\","
            << "\"swimlaneId\":\"" << esc(n.swimlaneId) << "\","
            << "\"layer\":" << n.layer << ","
            << "\"orderInLayer\":" << n.orderInLayer << ","
            << "\"x\":" << n.x << ","
            << "\"y\":" << n.y << ","
            << "\"width\":" << n.width << ","
            << "\"height\":" << n.height
            << "}";
    }
    out << "\n  ],\n";

    out << "  \"flows\": [\n";
    for (size_t i = 0; i < diagram.flows.size(); ++i) {
        const auto& f = diagram.flows[i];

        out << "    {"
            << "\"id\":\"" << esc(f.id) << "\","
            << "\"type\":\"" << flowTypeToString(f.type) << "\","
            << "\"fromId\":\"" << esc(f.fromId) << "\","
            << "\"toId\":\"" << esc(f.toId) << "\","
            << "\"guardLabel\":\"" << esc(f.guardLabel) << "\","
            << "\"objectLabel\":\"" << esc(f.objectLabel) << "\","
            << "\"isBackEdge\":" << (f.isBackEdge ? "true" : "false") << ","
            << "\"points\":[";

        for (size_t p = 0; p < f.points.size(); ++p) {
            out << "{\"x\":" << f.points[p].x << ",\"y\":" << f.points[p].y << "}";
            if (p + 1 < f.points.size()) out << ",";
        }

        out << "]}";
        if (i + 1 < diagram.flows.size()) out << ",";
        out << "\n";
    }
    out << "  ]\n";

    out << "}\n";
}