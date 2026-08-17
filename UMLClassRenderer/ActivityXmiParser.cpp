#include "ActivityXmiParser.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <iostream>

namespace {

    std::string readFile(const std::string& path) {
        std::ifstream in(path);
        if (!in.is_open()) {
            std::cerr << "[ActivityXmiParser] Cannot open file: " << path << "\n";
            return "";
        }

        std::ostringstream ss;
        ss << in.rdbuf();
        return ss.str();
    }

    std::string attr(const std::string& tag, const std::string& name) {
        std::regex r(name + R"(\s*=\s*["']([^"']*)["'])");
        std::smatch m;
        if (std::regex_search(tag, m, r)) return m[1].str();
        return "";
    }

    bool containsAny(const std::string& text,
        const std::initializer_list<std::string>& values) {
        for (const auto& v : values) {
            if (text.find(v) != std::string::npos) return true;
        }
        return false;
    }

    std::string cleanRef(std::string value) {
        if (!value.empty() && value[0] == '#') {
            value = value.substr(1);
        }

        size_t hashPos = value.find('#');
        if (hashPos != std::string::npos) {
            value = value.substr(hashPos + 1);
        }

        return value;
    }

    bool hasNode(const ActivityDiagram& diagram, const std::string& id) {
        for (const auto& node : diagram.nodes) {
            if (node.id == id) return true;
        }
        return false;
    }

    std::string readableNameFromId(std::string id) {
        if (id.rfind("node_", 0) == 0) {
            id = id.substr(5);
        }
        return id;
    }

    ActivityNodeKind detectNodeKind(const std::string& tagOrId) {
        std::string id = attr(tagOrId, "xmi:id");
        if (id.empty()) id = attr(tagOrId, "id");

        std::string name = attr(tagOrId, "name");

        std::string type = attr(tagOrId, "xmi:type");
        if (type.empty()) type = attr(tagOrId, "type");

        std::string combined = tagOrId + " " + type + " " + id + " " + name;

        if (containsAny(combined, {
            "uml:InitialNode", "InitialNode", "initialNode", "node_initial", "initial"
            })) {
            return ActivityNodeKind::Initial;
        }

        if (containsAny(combined, {
            "uml:DecisionNode", "DecisionNode", "decisionNode", "node_decision", "decision"
            })) {
            return ActivityNodeKind::Decision;
        }

        if (containsAny(combined, {
            "uml:MergeNode", "MergeNode", "mergeNode", "node_merge", "merge"
            })) {
            return ActivityNodeKind::Merge;
        }

        if (containsAny(combined, {
            "uml:ForkNode", "ForkNode", "forkNode", "node_fork", "fork"
            })) {
            return ActivityNodeKind::Fork;
        }

        if (containsAny(combined, {
            "uml:JoinNode", "JoinNode", "joinNode", "node_join", "join"
            })) {
            return ActivityNodeKind::Join;
        }

        if (containsAny(combined, {
            "uml:ActivityFinalNode", "ActivityFinalNode", "activityFinalNode",
            "node_final", "final"
            })) {
            return ActivityNodeKind::ActivityFinal;
        }

        if (containsAny(combined, {
            "uml:FlowFinalNode", "FlowFinalNode", "flowFinalNode"
            })) {
            return ActivityNodeKind::FlowFinal;
        }

        if (containsAny(combined, {
            "uml:ObjectNode", "ObjectNode", "objectNode",
            "CentralBufferNode", "DataStoreNode"
            })) {
            return ActivityNodeKind::Object;
        }

        return ActivityNodeKind::Action;
    }

    bool looksLikeFlow(const std::string& tag) {
        return containsAny(tag, { "ControlFlow", "ObjectFlow" }) ||
            (tag.find("source=") != std::string::npos &&
                tag.find("target=") != std::string::npos);
    }

    bool looksLikeSwimlane(const std::string& tag) {
        std::string type = attr(tag, "xmi:type");
        if (type.empty()) type = attr(tag, "type");

        return containsAny(tag, {
            "<partition",
            "<ownedPartition",
            "<group"
            }) || containsAny(type, {
                "uml:ActivityPartition",
                "ActivityPartition"
                }) || containsAny(tag, {
                    "ActivityPartition",
                    "activityPartition"
                    });
    }

    bool looksLikeActivityNode(const std::string& tag) {
        if (looksLikeFlow(tag)) return false;
        if (looksLikeSwimlane(tag)) return false;

        std::string id = attr(tag, "xmi:id");
        if (id.empty()) id = attr(tag, "id");

        std::string type = attr(tag, "xmi:type");
        if (type.empty()) type = attr(tag, "type");

        std::string combined = tag + " " + type + " " + id;

        if (containsAny(combined, {
            "uml:InitialNode",
            "uml:OpaqueAction",
            "uml:Action",
            "uml:CallBehaviorAction",
            "uml:CallOperationAction",
            "uml:DecisionNode",
            "uml:MergeNode",
            "uml:ForkNode",
            "uml:JoinNode",
            "uml:ActivityFinalNode",
            "uml:FlowFinalNode",
            "uml:ObjectNode",
            "InitialNode",
            "OpaqueAction",
            "Action",
            "DecisionNode",
            "MergeNode",
            "ForkNode",
            "JoinNode",
            "ActivityFinalNode",
            "FlowFinalNode",
            "ObjectNode"
            })) {
            return true;
        }

        if (id.rfind("node_", 0) == 0) {
            return true;
        }

        return false;
    }

} // namespace

ActivityDiagram parseActivityXmiFile(const std::string& inputPath) {
    ActivityDiagram diagram;
    std::string xml = readFile(inputPath);
    if (xml.empty()) return diagram;

    std::regex tagRegex(R"(<[^!?][^>]*>)");
    auto begin = std::sregex_iterator(xml.begin(), xml.end(), tagRegex);
    auto end = std::sregex_iterator();

    int generatedNodeId = 0;
    int generatedFlowId = 0;
    int generatedLaneId = 0;

    for (auto it = begin; it != end; ++it) {
        std::string tag = it->str();

        if (looksLikeSwimlane(tag)) {
            Swimlane lane;
            lane.id = attr(tag, "xmi:id");
            if (lane.id.empty()) lane.id = attr(tag, "id");
            if (lane.id.empty()) lane.id = "__lane_" + std::to_string(generatedLaneId++);

            lane.name = attr(tag, "name");
            lane.orderIndex = static_cast<int>(diagram.swimlanes.size());

            diagram.swimlanes.push_back(lane);
            continue;
        }

        if (looksLikeFlow(tag)) {
            std::string source = cleanRef(attr(tag, "source"));
            std::string target = cleanRef(attr(tag, "target"));

            if (source.empty()) source = cleanRef(attr(tag, "from"));
            if (target.empty()) target = cleanRef(attr(tag, "to"));

            if (source.empty() || target.empty()) continue;

            ActivityFlow flow;
            flow.id = attr(tag, "xmi:id");
            if (flow.id.empty()) flow.id = attr(tag, "id");
            if (flow.id.empty()) flow.id = "__flow_" + std::to_string(generatedFlowId++);

            flow.fromId = source;
            flow.toId = target;

            if (containsAny(tag, { "ObjectFlow", "objectFlow" }))
                flow.type = ActivityFlowType::Object;
            else
                flow.type = ActivityFlowType::Control;

            flow.guardLabel = attr(tag, "guard");
            flow.objectLabel = attr(tag, "name");

            diagram.flows.push_back(flow);
            continue;
        }

        if (looksLikeActivityNode(tag)) {
            ActivityNode node;

            node.id = attr(tag, "xmi:id");
            if (node.id.empty()) node.id = attr(tag, "id");
            if (node.id.empty()) node.id = "__node_" + std::to_string(generatedNodeId++);

            node.id = cleanRef(node.id);

            node.name = attr(tag, "name");
            if (node.name.empty()) {
                node.name = readableNameFromId(node.id);
            }

            node.kind = detectNodeKind(tag);

            if (node.kind == ActivityNodeKind::Fork || node.kind == ActivityNodeKind::Join) {
                node.horizontalBar = true;
            }

            node.swimlaneId = cleanRef(attr(tag, "inPartition"));
            if (node.swimlaneId.empty()) node.swimlaneId = cleanRef(attr(tag, "partition"));

            std::string x = attr(tag, "x");
            std::string y = attr(tag, "y");
            if (!x.empty() && !y.empty()) {
                node.xmiX = std::stod(x);
                node.xmiY = std::stod(y);
                node.hasXmiCoords = true;
            }

            diagram.nodes.push_back(node);
        }
    }

    for (const auto& flow : diagram.flows) {
        if (!hasNode(diagram, flow.fromId)) {
            ActivityNode node;
            node.id = flow.fromId;
            node.name = readableNameFromId(flow.fromId);
            node.kind = detectNodeKind(flow.fromId);

            if (node.kind == ActivityNodeKind::Fork || node.kind == ActivityNodeKind::Join) {
                node.horizontalBar = true;
            }

            diagram.nodes.push_back(node);
        }

        if (!hasNode(diagram, flow.toId)) {
            ActivityNode node;
            node.id = flow.toId;
            node.name = readableNameFromId(flow.toId);
            node.kind = detectNodeKind(flow.toId);

            if (node.kind == ActivityNodeKind::Fork || node.kind == ActivityNodeKind::Join) {
                node.horizontalBar = true;
            }

            diagram.nodes.push_back(node);
        }
    }

    return diagram;
}