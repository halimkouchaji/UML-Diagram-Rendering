#include "SvgRenderer.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cmath>

using json = nlohmann::json;

namespace {

    constexpr double HEADER_H = 50.0;   // name compartment height (matches NodeSizer's fixed header allowance)
    constexpr double ROW_H = 20.0;   // per attribute/operation row height
    constexpr double MARGIN = 40.0;   // canvas padding around the whole diagram
    constexpr double FONT_NAME = 14.0;
    constexpr double FONT_MEMBER = 12.0;

    struct Member {
        std::string name;
        std::string type;        // for attributes: type; for operations: returnType
        std::string visibility;
    };

    struct ClassBox {
        std::string id, name;
        double x, y, width, height;
        std::vector<Member> attributes;
        std::vector<Member> operations;
    };

    struct Pt { double x, y; };

    struct Edge {
        std::string id, type, from, to;
        std::vector<Pt> points;
    };

    std::string visSymbol(const std::string& v) {
        if (v == "public") return "+";
        if (v == "private") return "-";
        if (v == "protected") return "#";
        if (v == "package") return "~";
        return "";
    }

    std::string escapeXml(const std::string& s) {
        std::string out;
        out.reserve(s.size());
        for (char c : s) {
            switch (c) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            default:  out += c;
            }
        }
        return out;
    }

    // Which marker (if any) each edge type uses, and which end it attaches to.
    struct EdgeStyle {
        bool dashed;
        std::string markerId; // empty = no marker
        bool markerAtStart;   // true = marker-start (near "from"), false = marker-end (near "to")
    };

    EdgeStyle styleFor(const std::string& type) {
        if (type == "generalization") return { false, "triangle-hollow", false };
        if (type == "realization")    return { true,  "triangle-hollow", false };
        if (type == "aggregation")    return { false, "diamond-hollow",  true };
        if (type == "composition")    return { false, "diamond-filled",  true };
        if (type == "dependency")     return { true,  "arrow-open",      false };
        return { false, "arrow-open", false }; // association
    }

    std::string defsBlock() {
        std::ostringstream s;
        s << "<defs>\n";
        // Hollow triangle (generalization / realization)
        s << R"(<marker id="triangle-hollow" markerWidth="16" markerHeight="12" refX="15" refY="6" orient="auto" markerUnits="userSpaceOnUse">)"
            << R"(<path d="M1,1 L15,6 L1,11 Z" fill="white" stroke="black" stroke-width="1"/></marker>)" << "\n";
        // Hollow diamond (aggregation)
        s << R"(<marker id="diamond-hollow" markerWidth="18" markerHeight="10" refX="1" refY="5" orient="auto" markerUnits="userSpaceOnUse">)"
            << R"(<path d="M1,5 L9,1 L17,5 L9,9 Z" fill="white" stroke="black" stroke-width="1"/></marker>)" << "\n";
        // Filled diamond (composition)
        s << R"(<marker id="diamond-filled" markerWidth="18" markerHeight="10" refX="1" refY="5" orient="auto" markerUnits="userSpaceOnUse">)"
            << R"(<path d="M1,5 L9,1 L17,5 L9,9 Z" fill="black" stroke="black" stroke-width="1"/></marker>)" << "\n";
        // Open arrow (association / dependency)
        s << R"(<marker id="arrow-open" markerWidth="14" markerHeight="12" refX="12" refY="6" orient="auto" markerUnits="userSpaceOnUse">)"
            << R"(<path d="M1,1 L12,6 L1,11" fill="none" stroke="black" stroke-width="1.3"/></marker>)" << "\n";
        s << "</defs>\n";
        return s.str();
    }

} // namespace

void renderToSvg(const std::string& jsonPath, const std::string& svgPath) {
    std::ifstream in(jsonPath);
    if (!in) {
        std::cerr << "Failed to open input JSON: " << jsonPath << std::endl;
        return;
    }
    json j;
    in >> j;

    std::vector<ClassBox> classes;
    for (const auto& c : j.at("classes")) {
        ClassBox cb;
        cb.id = c.at("id").get<std::string>();
        cb.name = c.at("name").get<std::string>();
        cb.x = c.at("x").get<double>();
        cb.y = c.at("y").get<double>();
        cb.width = c.at("width").get<double>();
        cb.height = c.at("height").get<double>();
        for (const auto& a : c.at("attributes")) {
            cb.attributes.push_back({ a.at("name").get<std::string>(),
                                       a.at("type").get<std::string>(),
                                       a.at("visibility").get<std::string>() });
        }
        for (const auto& o : c.at("operations")) {
            cb.operations.push_back({ o.at("name").get<std::string>(),
                                       o.at("returnType").get<std::string>(),
                                       o.at("visibility").get<std::string>() });
        }
        classes.push_back(std::move(cb));
    }

    std::vector<Edge> edges;
    for (const auto& e : j.at("edges")) {
        Edge ed;
        ed.id = e.at("id").get<std::string>();
        ed.type = e.at("type").get<std::string>();
        ed.from = e.at("from").get<std::string>();
        ed.to = e.at("to").get<std::string>();
        for (const auto& p : e.at("points")) {
            ed.points.push_back({ p.at("x").get<double>(), p.at("y").get<double>() });
        }
        edges.push_back(std::move(ed));
    }

    // Canvas size = bounding box of all classes, plus margin.
    double maxX = 0, maxY = 0;
    for (const auto& c : classes) {
        maxX = std::max(maxX, c.x + c.width);
        maxY = std::max(maxY, c.y + c.height);
    }
    double canvasW = maxX + 2 * MARGIN;
    double canvasH = maxY + 2 * MARGIN;

    std::ostringstream svg;
    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 " << canvasW << " " << canvasH
        << "\" font-family=\"Helvetica, Arial, sans-serif\">\n";
    svg << "<rect x=\"0\" y=\"0\" width=\"" << canvasW << "\" height=\"" << canvasH << "\" fill=\"white\"/>\n";
    svg << defsBlock();

    // --- Edges first, so class boxes sit visually on top of line ends ---
    svg << "<g stroke=\"black\" fill=\"none\">\n";
    for (const auto& e : edges) {
        if (e.points.size() < 2) continue;
        EdgeStyle style = styleFor(e.type);

        std::ostringstream pts;
        for (size_t i = 0; i < e.points.size(); ++i) {
            if (i) pts << " ";
            pts << (e.points[i].x + MARGIN) << "," << (e.points[i].y + MARGIN);
        }

        svg << "<polyline points=\"" << pts.str() << "\" stroke=\"black\" stroke-width=\"1.4\" fill=\"none\"";
        if (style.dashed) svg << " stroke-dasharray=\"6,4\"";
        if (!style.markerId.empty()) {
            if (style.markerAtStart) svg << " marker-start=\"url(#" << style.markerId << ")\"";
            else                     svg << " marker-end=\"url(#" << style.markerId << ")\"";
        }
        svg << "/>\n";
    }
    svg << "</g>\n";

    // --- Class boxes ---
    for (const auto& c : classes) {
        double bx = c.x + MARGIN, by = c.y + MARGIN;
        double attrsH = ROW_H * c.attributes.size();
        double opsH = ROW_H * c.operations.size();

        svg << "<g>\n";
        // Outer box
        svg << "<rect x=\"" << bx << "\" y=\"" << by << "\" width=\"" << c.width
            << "\" height=\"" << c.height << "\" fill=\"#fdfdfd\" stroke=\"black\" stroke-width=\"1.2\"/>\n";

        // Name compartment
        svg << "<text x=\"" << (bx + c.width / 2) << "\" y=\"" << (by + HEADER_H / 2 + FONT_NAME / 3)
            << "\" font-size=\"" << FONT_NAME << "\" font-weight=\"bold\" text-anchor=\"middle\">"
            << escapeXml(c.name) << "</text>\n";

        // Divider under name (only if there's anything below)
        if (!c.attributes.empty() || !c.operations.empty()) {
            svg << "<line x1=\"" << bx << "\" y1=\"" << (by + HEADER_H) << "\" x2=\"" << (bx + c.width)
                << "\" y2=\"" << (by + HEADER_H) << "\" stroke=\"black\" stroke-width=\"1\"/>\n";
        }

        // Attributes
        double rowY = by + HEADER_H;
        for (const auto& a : c.attributes) {
            std::string line = visSymbol(a.visibility) + " " + a.name + " : " + a.type;
            svg << "<text x=\"" << (bx + 8) << "\" y=\"" << (rowY + ROW_H * 0.68)
                << "\" font-size=\"" << FONT_MEMBER << "\">" << escapeXml(line) << "</text>\n";
            rowY += ROW_H;
        }

        // Divider between attributes and operations (only needed if both sections exist)
        if (!c.attributes.empty() && !c.operations.empty()) {
            svg << "<line x1=\"" << bx << "\" y1=\"" << rowY << "\" x2=\"" << (bx + c.width)
                << "\" y2=\"" << rowY << "\" stroke=\"black\" stroke-width=\"1\"/>\n";
        }

        // Operations
        for (const auto& o : c.operations) {
            std::string ret = o.type.empty() ? "" : (" : " + o.type);
            std::string line = visSymbol(o.visibility) + " " + o.name + "()" + ret;
            svg << "<text x=\"" << (bx + 8) << "\" y=\"" << (rowY + ROW_H * 0.68)
                << "\" font-size=\"" << FONT_MEMBER << "\">" << escapeXml(line) << "</text>\n";
            rowY += ROW_H;
        }

        svg << "</g>\n";
    }

    svg << "</svg>\n";

    std::ofstream out(svgPath);
    if (!out) {
        std::cerr << "Failed to open output SVG: " << svgPath << std::endl;
        return;
    }
    out << svg.str();
    std::cout << "Wrote diagram to " << svgPath << std::endl;
}