#include "EdgeRouter.h"
#include <unordered_map>

namespace {
    struct Box { double x, y, w, h; };

    Point topCenter(const Box& b) { return { b.x + b.w / 2.0, b.y }; }
    Point bottomCenter(const Box& b) { return { b.x + b.w / 2.0, b.y + b.h }; }
    Point leftCenter(const Box& b) { return { b.x, b.y + b.h / 2.0 }; }
    Point rightCenter(const Box& b) { return { b.x + b.w, b.y + b.h / 2.0 }; }
}

void routeEdges(ClassDiagram& diagram) {
    std::unordered_map<std::string, Box> boxById;
    for (const auto& cls : diagram.classes) {
        boxById[cls.id] = { cls.x, cls.y, cls.width, cls.height };
    }

    for (auto& edge : diagram.edges) {
        auto fromIt = boxById.find(edge.fromId);
        auto toIt = boxById.find(edge.toId);
        if (fromIt == boxById.end() || toIt == boxById.end()) continue;

        const Box& from = fromIt->second;
        const Box& to = toIt->second;

        edge.points.clear();

        if (edge.type == EdgeType::Generalization || edge.type == EdgeType::Realization) {
            // Child's top-center up to parent's bottom-center.
            edge.points.push_back(topCenter(from));
            edge.points.push_back(bottomCenter(to));
        }
        else {
            // Association-style: connect whichever sides face each other.
            bool toIsRight = (to.x > from.x);
            bool toIsBelow = (to.y > from.y);

            Point start, end;
            if (std::abs(to.x - from.x) > std::abs(to.y - from.y)) {
                start = toIsRight ? rightCenter(from) : leftCenter(from);
                end = toIsRight ? leftCenter(to) : rightCenter(to);
            }
            else {
                start = toIsBelow ? bottomCenter(from) : topCenter(from);
                end = toIsBelow ? topCenter(to) : bottomCenter(to);
            }
            edge.points.push_back(start);
            edge.points.push_back(end);
        }
    }
}