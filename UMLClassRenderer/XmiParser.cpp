#include "XmiParser.h"
#include <pugixml.hpp>
#include <iostream>
#include <unordered_map>
#include <sstream>

namespace {

    Visibility parseVisibility(const std::string& v) {
        if (v == "public")    return Visibility::Public;
        if (v == "protected") return Visibility::Protected;
        if (v == "package")   return Visibility::Package;
        return Visibility::Private;
    }

    // Info about one "end" of an association, wherever it's defined.
    struct EndInfo {
        std::string classId;       // which class this end connects to
        std::string aggregation;   // "none" | "shared" | "composite"
    };

} // namespace

ClassDiagram parseXmiFile(const std::string& filepath) {
    ClassDiagram diagram;

    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(filepath.c_str());
    if (!result) {
        std::cerr << "Failed to load XMI file: " << result.description() << std::endl;
        return diagram;
    }

    pugi::xml_node root = doc.child("xmi:XMI");
    if (!root) { std::cerr << "No xmi:XMI root element found.\n"; return diagram; }

    pugi::xml_node model = root.child("uml:Model");
    if (!model) { std::cerr << "No uml:Model element found.\n"; return diagram; }

    // Registry: attribute/end id -> EndInfo, built from ownedAttribute
    // elements nested inside classes (Pattern B — the common real-world case).
    std::unordered_map<std::string, EndInfo> endRegistry;

    // --- Pass 1: classes + interfaces, attributes, operations ---
    for (pugi::xml_node el : model.children("packagedElement")) {
        std::string type = el.attribute("xmi:type").as_string();
        if (type == "uml:Class" || type == "uml:Interface") {
            ClassNode node;
            node.id = el.attribute("xmi:id").as_string();
            node.name = el.attribute("name").as_string();
            node.isInterface = (type == "uml:Interface");

            for (pugi::xml_node attrEl : el.children("ownedAttribute")) {
                Attribute attr;
                attr.name = attrEl.attribute("name").as_string();
                attr.type = attrEl.attribute("type").as_string();
                attr.visibility = parseVisibility(attrEl.attribute("visibility").as_string());
                node.attributes.push_back(attr);

                // Register this attribute as a possible association end
                // (Pattern B): the owner of this ownedAttribute is THIS
                // class; the "type" attribute points at the other class.
                std::string attrId = attrEl.attribute("xmi:id").as_string();
                std::string agg = attrEl.attribute("aggregation").as_string();
                if (agg.empty()) agg = "none";
                if (!attrId.empty()) {
                    endRegistry[attrId] = EndInfo{ node.id, agg };
                }
            }

            for (pugi::xml_node opEl : el.children("ownedOperation")) {
                Operation op;
                op.name = opEl.attribute("name").as_string();
                op.visibility = parseVisibility(opEl.attribute("visibility").as_string());
                node.operations.push_back(op);
            }

            diagram.classes.push_back(std::move(node));
        }
    }

    // --- Pass 2: generalizations (inheritance) ---
    for (pugi::xml_node el : model.children("packagedElement")) {
        std::string type = el.attribute("xmi:type").as_string();
        if (type == "uml:Class" || type == "uml:Interface") {
            std::string childId = el.attribute("xmi:id").as_string();
            for (pugi::xml_node genEl : el.children("generalization")) {
                Edge edge;
                edge.id = genEl.attribute("xmi:id").as_string();
                edge.type = EdgeType::Generalization;
                edge.fromId = childId;
                edge.toId = genEl.attribute("general").as_string();
                diagram.edges.push_back(edge);
            }
        }
    }

    // --- Pass 3: realization / interface realization ---
    // Handles BOTH common shapes:
    //  (a) nested inside the realizing class: <interfaceRealization supplier="ifaceId"/>
    //  (b) top-level packagedElement: xmi:type="uml:Realization" / "uml:InterfaceRealization"
    //      with client="clsId" supplier="ifaceId"
    for (pugi::xml_node el : model.children("packagedElement")) {
        std::string type = el.attribute("xmi:type").as_string();
        if (type == "uml:Class" || type == "uml:Interface") {
            std::string clientId = el.attribute("xmi:id").as_string();
            for (pugi::xml_node reEl : el.children("interfaceRealization")) {
                std::string supplier = reEl.attribute("supplier").as_string();
                if (supplier.empty()) {
                    supplier = reEl.attribute("contract").as_string();  // older/alternate naming
                }
                if (supplier.empty()) {
                    // some tools nest it as a child element instead of an attribute
                    pugi::xml_node supEl = reEl.child("supplier");
                    if (supEl) supplier = supEl.attribute("xmi:idref").as_string();
                }
                if (supplier.empty()) continue;

                Edge edge;
                edge.id = reEl.attribute("xmi:id").as_string();
                edge.type = EdgeType::Realization;
                edge.fromId = clientId;   // the class
                edge.toId = supplier;     // the interface
                diagram.edges.push_back(edge);
            }
        }
        else if (type == "uml:Realization" || type == "uml:InterfaceRealization") {
            std::string client = el.attribute("client").as_string();
            std::string supplier = el.attribute("supplier").as_string();
            if (client.empty() || supplier.empty()) continue;

            Edge edge;
            edge.id = el.attribute("xmi:id").as_string();
            edge.type = EdgeType::Realization;
            edge.fromId = client;
            edge.toId = supplier;
            diagram.edges.push_back(edge);
        }
    }

    // --- Pass 4: dependency ---
    for (pugi::xml_node el : model.children("packagedElement")) {
        std::string type = el.attribute("xmi:type").as_string();
        if (type == "uml:Dependency") {
            std::string client = el.attribute("client").as_string();
            std::string supplier = el.attribute("supplier").as_string();
            if (client.empty() || supplier.empty()) continue;

            Edge edge;
            edge.id = el.attribute("xmi:id").as_string();
            edge.type = EdgeType::Dependency;
            edge.fromId = client;    // the dependent class
            edge.toId = supplier;    // the class it depends on
            diagram.edges.push_back(edge);
        }
    }

    // --- Pass 5: associations (handles Pattern A and Pattern B for aggregation) ---
    for (pugi::xml_node el : model.children("packagedElement")) {
        std::string type = el.attribute("xmi:type").as_string();
        if (type != "uml:Association") continue;

        Edge edge;
        edge.id = el.attribute("xmi:id").as_string();
        edge.type = EdgeType::Association;

        // Pattern A: ends are <ownedEnd> children of the association itself.
        std::vector<EndInfo> ends;
        for (pugi::xml_node endEl : el.children("ownedEnd")) {
            std::string classId = endEl.attribute("type").as_string();
            std::string agg = endEl.attribute("aggregation").as_string();
            if (agg.empty()) agg = "none";
            ends.push_back(EndInfo{ classId, agg });
        }

        // Pattern B: memberEnd references ids that live in endRegistry
        // (ownedAttribute inside a class), rather than ownedEnd children.
        if (ends.empty()) {
            std::istringstream iss(el.attribute("memberEnd").as_string());
            std::string idref;
            while (iss >> idref) {
                auto it = endRegistry.find(idref);
                if (it != endRegistry.end()) ends.push_back(it->second);
            }
        }

        if (ends.size() < 2) continue; // not enough info to connect two classes

        // Determine part/whole: whichever end has aggregation != "none"
        // is the PART; the other end is the WHOLE (diamond goes there).
        std::string aggregationKind = "none";
        std::string partId, wholeId;

        if (ends[0].aggregation != "none") {
            partId = ends[0].classId; wholeId = ends[1].classId;
            aggregationKind = ends[0].aggregation;
        }
        else if (ends[1].aggregation != "none") {
            partId = ends[1].classId; wholeId = ends[0].classId;
            aggregationKind = ends[1].aggregation;
        }
        else {
            partId = ends[0].classId; wholeId = ends[1].classId;
        }

        edge.fromId = partId;   // part
        edge.toId = wholeId;    // whole (diamond drawn here, if any)

        if (aggregationKind == "composite") edge.type = EdgeType::Composition;
        else if (aggregationKind == "shared") edge.type = EdgeType::Aggregation;

        diagram.edges.push_back(edge);
    }

    return diagram;
}