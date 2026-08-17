#include "SvgExporter.h"
#include <fstream>
#include <sstream>
#include <cmath>
#include <unordered_map>
#include <iostream>

namespace {

    std::string visibilitySymbol(Visibility v) {
        switch (v) {
        case Visibility::Public:    return "+";
        case Visibility::Private:   return "-";
        case Visibility::Protected: return "#";
        case Visibility::Package:   return "~";
        }
        return "-";
    }

    std::string escapeXml(const std::string& s) {
        std::string out;
        for (char c : s) {
            if (c == '&') out += "&amp;";
            else if (c == '<') out += "&lt;";
            else if (c == '>') out += "&gt;";
            else out += c;
        }
        return out;
    }

    // Returns points for a triangle marker (hollow), tip at 'to', pointing
    // back toward 'from'. Used for Generalization and Realization.
    std::string triangleMarker(const Point& from, const Point& to, bool filled) {
        double dx = to.x - from.x, dy = to.y - from.y;
        double len = std::sqrt(dx * dx + dy * dy);
        if (len < 1e-6) return "";
        dx /= len; dy /= len;
        double size = 14.0, halfW = 7.0;
        double baseX = to.x - dx * size, baseY = to.y - dy * size;
        double perpX = -dy, perpY = dx;

        std::ostringstream pts;
        pts << to.x << "," << to.y << " "
            << (baseX + perpX * halfW) << "," << (baseY + perpY * halfW) << " "
            << (baseX - perpX * halfW) << "," << (baseY - perpY * halfW);

        std::ostringstream svg;
        svg << "<polygon points=\"" << pts.str() << "\" fill=\""
            << (filled ? "black" : "white") << "\" stroke=\"black\" stroke-width=\"1.5\"/>";
        return svg.str();
    }

    // Diamond marker for Aggregation (hollow) / Composition (filled), at 'to' end.
    std::string diamondMarker(const Point& from, const Point& to, bool filled) {
        double dx = to.x - from.x, dy = to.y - from.y;
        double len = std::sqrt(dx * dx + dy * dy);
        if (len < 1e-6) return "";
        dx /= len; dy /= len;
        double size = 16.0, halfW = 6.0;
        double perpX = -dy, perpY = dx;
        double midX = to.x - dx * size / 2, midY = to.y - dy * size / 2;
        double backX = to.x - dx * size, backY = to.y - dy * size;

        std::ostringstream pts;
        pts << to.x << "," << to.y << " "
            << (midX + perpX * halfW) << "," << (midY + perpY * halfW) << " "
            << backX << "," << backY << " "
            << (midX - perpX * halfW) << "," << (midY - perpY * halfW);

        std::ostringstream svg;
        svg << "<polygon points=\"" << pts.str() << "\" fill=\""
            << (filled ? "black" : "white") << "\" stroke=\"black\" stroke-width=\"1.5\"/>";
        return svg.str();
    }

    // Open arrowhead (V shape, no fill) for Dependency.
    std::string openArrowMarker(const Point& from, const Point& to) {
        double dx = to.x - from.x, dy = to.y - from.y;
        double len = std::sqrt(dx * dx + dy * dy);
        if (len < 1e-6) return "";
        dx /= len; dy /= len;
        double size = 12.0, halfW = 6.0;
        double baseX = to.x - dx * size, baseY = to.y - dy * size;
        double perpX = -dy, perpY = dx;

        std::ostringstream svg;
        svg << "<polyline points=\""
            << (baseX + perpX * halfW) << "," << (baseY + perpY * halfW) << " "
            << to.x << "," << to.y << " "
            << (baseX - perpX * halfW) << "," << (baseY - perpY * halfW)
            << "\" fill=\"none\" stroke=\"black\" stroke-width=\"1.5\"/>";
        return svg.str();
    }

} // namespace

void exportToSvg(const ClassDiagram& diagram, const std::string& outPath) {
    double maxX = 0, maxY = 0;
    for (const auto& cls : diagram.classes) {
        maxX = std::max(maxX, cls.x + cls.width);
        maxY = std::max(maxY, cls.y + cls.height);
    }
    maxX += 60; maxY += 60;

    std::ostringstream svg;
    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << maxX
        << "\" height=\"" << maxY << "\" viewBox=\"0 0 " << maxX << " " << maxY << "\">\n";
    svg << "<rect width=\"100%\" height=\"100%\" fill=\"white\"/>\n";

    // --- Edges first, so boxes draw on top of line endpoints cleanly ---
  // --- Edges first, so boxes draw on top of line endpoints cleanly ---
    for (const auto& e : diagram.edges) {
        if (e.points.size() < 2) continue;

        bool dashed = (e.type == EdgeType::Realization || e.type == EdgeType::Dependency);

        // Draw the FULL path through every point (handles both simple
        // 2-point direct edges AND multi-point lane-routed edges).
        std::ostringstream ptsStream;
        for (size_t i = 0; i < e.points.size(); ++i) {
            if (i > 0) ptsStream << " ";
            ptsStream << e.points[i].x << "," << e.points[i].y;
        }
        svg << "<polyline points=\"" << ptsStream.str()
            << "\" fill=\"none\" stroke=\"black\" stroke-width=\"1.5\""
            << (dashed ? " stroke-dasharray=\"6,4\"" : "") << "/>\n";

        // Arrowhead direction must come from the LAST SEGMENT (second-to-last
        // point -> last point), not the overall first->last, since a
        // lane-routed edge's final approach angle differs from its overall span.
        const Point& dirFrom = e.points[e.points.size() - 2];
        const Point& dirTo = e.points.back();

        switch (e.type) {
        case EdgeType::Generalization:
            svg << triangleMarker(dirFrom, dirTo, false) << "\n";
            break;
        case EdgeType::Realization:
            svg << triangleMarker(dirFrom, dirTo, false) << "\n";
            break;
        case EdgeType::Aggregation:
            svg << diamondMarker(dirFrom, dirTo, false) << "\n";
            break;
        case EdgeType::Composition:
            svg << diamondMarker(dirFrom, dirTo, true) << "\n";
            break;
        case EdgeType::Dependency:
            svg << openArrowMarker(dirFrom, dirTo) << "\n";
            break;
        default:
            break; // plain Association: no marker
        }
    }

    // --- Class / interface boxes ---
    for (const auto& cls : diagram.classes) {
        double x = cls.x, y = cls.y, w = cls.width, h = cls.height;
        svg << "<rect x=\"" << x << "\" y=\"" << y << "\" width=\"" << w
            << "\" height=\"" << h << "\" fill=\"white\" stroke=\"black\" stroke-width=\"1.5\"/>\n";

        double nameHeight = 30.0;
        double textY = y + 20;

        if (cls.isInterface) {
            svg << "<text x=\"" << (x + w / 2) << "\" y=\"" << (textY - 14)
                << "\" font-size=\"11\" font-style=\"italic\" text-anchor=\"middle\">&lt;&lt;interface&gt;&gt;</text>\n";
        }
        svg << "<text x=\"" << (x + w / 2) << "\" y=\"" << textY
            << "\" font-size=\"13\" font-weight=\"bold\" text-anchor=\"middle\">"
            << escapeXml(cls.name) << "</text>\n";

        double lineY = y + nameHeight;
        svg << "<line x1=\"" << x << "\" y1=\"" << lineY << "\" x2=\"" << (x + w)
            << "\" y2=\"" << lineY << "\" stroke=\"black\" stroke-width=\"1\"/>\n";

        double cursorY = lineY + 16;
        for (const auto& a : cls.attributes) {
            std::string line = visibilitySymbol(a.visibility) + " " + a.name;
            if (!a.type.empty()) line += ": " + a.type;
            svg << "<text x=\"" << (x + 8) << "\" y=\"" << cursorY
                << "\" font-size=\"12\">" << escapeXml(line) << "</text>\n";
            cursorY += 16;
        }

        double attrSectionBottom = std::max(lineY + 16, cursorY);
        svg << "<line x1=\"" << x << "\" y1=\"" << attrSectionBottom << "\" x2=\"" << (x + w)
            << "\" y2=\"" << attrSectionBottom << "\" stroke=\"black\" stroke-width=\"1\"/>\n";

        cursorY = attrSectionBottom + 16;
        for (const auto& op : cls.operations) {
            std::string line = visibilitySymbol(op.visibility) + " " + op.name + "()";
            svg << "<text x=\"" << (x + 8) << "\" y=\"" << cursorY
                << "\" font-size=\"12\">" << escapeXml(line) << "</text>\n";
            cursorY += 16;
        }
    }

    svg << "</svg>\n";

    std::ofstream file(outPath);
    if (!file) {
        std::cerr << "Failed to open output file: " << outPath << std::endl;
        return;
    }
    file << svg.str();
    std::cout << "Wrote SVG to " << outPath << std::endl;
}