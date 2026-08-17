#include "ActivitySvgRenderer.h"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace {

    std::string esc(const std::string& s) {
        std::string out;
        for (char c : s) {
            switch (c) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            default: out += c; break;
            }
        }
        return out;
    }

    double cx(const ActivityNode& n) { return n.x + n.width / 2.0; }
    double cy(const ActivityNode& n) { return n.y + n.height / 2.0; }

    void renderText(std::ostream& out, double x, double y, const std::string& text,
        const std::string& anchor = "middle") {
        if (text.empty()) return;
        out << "<text x=\"" << x << "\" y=\"" << y
            << "\" text-anchor=\"" << anchor
            << "\" dominant-baseline=\"middle\" font-family=\"Arial\" font-size=\"12\">"
            << esc(text) << "</text>\n";
    }

    void renderPolyline(std::ostream& out, const ActivityFlow& f) {
        if (f.points.size() < 2) return;

        out << "<polyline points=\"";
        for (const auto& p : f.points) {
            out << p.x << "," << p.y << " ";
        }

        out << "\" fill=\"none\" stroke=\"#222\" stroke-width=\"1.6\" ";

        if (f.type == ActivityFlowType::Object) {
            out << "stroke-dasharray=\"6 4\" ";
        }

        out << "marker-end=\"url(#arrow)\" />\n";
    }

    void renderNode(std::ostream& out, const ActivityNode& n) {
        if (n.isSpacer) return;

        switch (n.kind) {
        case ActivityNodeKind::Initial:
            out << "<circle cx=\"" << cx(n) << "\" cy=\"" << cy(n)
                << "\" r=\"10\" fill=\"#111\" />\n";
            break;

        case ActivityNodeKind::ActivityFinal:
            out << "<circle cx=\"" << cx(n) << "\" cy=\"" << cy(n)
                << "\" r=\"15\" fill=\"white\" stroke=\"#111\" stroke-width=\"2\" />\n";
            out << "<circle cx=\"" << cx(n) << "\" cy=\"" << cy(n)
                << "\" r=\"9\" fill=\"#111\" />\n";
            break;

        case ActivityNodeKind::FlowFinal:
            out << "<circle cx=\"" << cx(n) << "\" cy=\"" << cy(n)
                << "\" r=\"15\" fill=\"white\" stroke=\"#111\" stroke-width=\"2\" />\n";
            out << "<line x1=\"" << cx(n) - 8 << "\" y1=\"" << cy(n) - 8
                << "\" x2=\"" << cx(n) + 8 << "\" y2=\"" << cy(n) + 8
                << "\" stroke=\"#111\" stroke-width=\"2\" />\n";
            out << "<line x1=\"" << cx(n) + 8 << "\" y1=\"" << cy(n) - 8
                << "\" x2=\"" << cx(n) - 8 << "\" y2=\"" << cy(n) + 8
                << "\" stroke=\"#111\" stroke-width=\"2\" />\n";
            break;

        case ActivityNodeKind::Decision:
        case ActivityNodeKind::Merge:
            out << "<polygon points=\""
                << cx(n) << "," << n.y << " "
                << n.x + n.width << "," << cy(n) << " "
                << cx(n) << "," << n.y + n.height << " "
                << n.x << "," << cy(n)
                << "\" fill=\"white\" stroke=\"#222\" stroke-width=\"1.5\" />\n";
            renderText(out, cx(n), cy(n), n.name);
            break;

        case ActivityNodeKind::Fork:
        case ActivityNodeKind::Join:
            out << "<rect x=\"" << n.x << "\" y=\"" << n.y
                << "\" width=\"" << n.width << "\" height=\"" << n.height
                << "\" fill=\"#111\" />\n";
            break;

        case ActivityNodeKind::Object:
            out << "<rect x=\"" << n.x << "\" y=\"" << n.y
                << "\" width=\"" << n.width << "\" height=\"" << n.height
                << "\" fill=\"#f8fbff\" stroke=\"#222\" stroke-width=\"1.4\" />\n";
            renderText(out, cx(n), cy(n), n.name);
            break;

        case ActivityNodeKind::Action:
        default:
            out << "<rect x=\"" << n.x << "\" y=\"" << n.y
                << "\" width=\"" << n.width << "\" height=\"" << n.height
                << "\" rx=\"8\" ry=\"8\" fill=\"white\" stroke=\"#222\" stroke-width=\"1.4\" />\n";
            renderText(out, cx(n), cy(n), n.name);
            break;
        }
    }

} // namespace

bool renderActivitySvg(const ActivityDiagram& diagram,
    const ActivityLayoutContext& ctx,
    const std::string& outputPath) {
    std::ofstream out(outputPath);
    if (!out.is_open()) return false;

    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << ctx.diagramWidth
        << "\" height=\"" << ctx.diagramHeight
        << "\" viewBox=\"0 0 " << ctx.diagramWidth << " " << ctx.diagramHeight << "\">\n";

    out << "<defs>\n";
    out << "<marker id=\"arrow\" markerWidth=\"10\" markerHeight=\"10\" refX=\"9\" refY=\"3\" orient=\"auto\" markerUnits=\"strokeWidth\">\n";
    out << "<path d=\"M0,0 L0,6 L9,3 z\" fill=\"#222\" />\n";
    out << "</marker>\n";
    out << "</defs>\n";

    out << "<rect width=\"100%\" height=\"100%\" fill=\"white\" />\n";

    out << "<g id=\"swimlanes\">\n";
    for (const auto& lane : diagram.swimlanes) {
        out << "<rect x=\"" << lane.x << "\" y=\"" << lane.y
            << "\" width=\"" << lane.width << "\" height=\"" << lane.height
            << "\" fill=\"#f6f7f9\" stroke=\"#999\" stroke-width=\"1\" />\n";

        out << "<rect x=\"" << lane.x << "\" y=\"" << lane.y
            << "\" width=\"" << lane.width << "\" height=\"" << ctx.laneHeaderSize
            << "\" fill=\"#e9edf3\" stroke=\"#999\" stroke-width=\"1\" />\n";

        renderText(out, lane.x + 12.0, lane.y + ctx.laneHeaderSize / 2.0, lane.name, "start");
    }
    out << "</g>\n";

    out << "<g id=\"edges-control\">\n";
    for (const auto& flow : diagram.flows) {
        if (flow.type == ActivityFlowType::Control) renderPolyline(out, flow);
    }
    out << "</g>\n";

    out << "<g id=\"edges-object\">\n";
    for (const auto& flow : diagram.flows) {
        if (flow.type == ActivityFlowType::Object) renderPolyline(out, flow);
    }
    out << "</g>\n";

    out << "<g id=\"nodes\">\n";
    for (const auto& node : diagram.nodes) {
        renderNode(out, node);
    }
    out << "</g>\n";

    out << "<g id=\"labels\">\n";
    for (const auto& label : ctx.edgeLabels) {
        out << "<rect x=\"" << label.x << "\" y=\"" << label.y
            << "\" width=\"" << label.width << "\" height=\"" << label.height
            << "\" fill=\"white\" opacity=\"0.85\" />\n";

        renderText(out,
            label.x + label.width / 2.0,
            label.y + label.height / 2.0,
            label.text);
    }
    out << "</g>\n";

    out << "</svg>\n";
    return true;
}