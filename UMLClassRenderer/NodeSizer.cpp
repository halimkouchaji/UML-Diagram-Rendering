#include "NodeSizer.h"
#include <algorithm>

namespace {
    const double kCharWidth = 7.0;   // approx px per character (12pt font)
    const double kLineHeight = 18.0;  // px per text line
    const double kPaddingX = 12.0;  // left+right padding per side
    const double kPaddingY = 6.0;   // top+bottom padding per compartment
    const double kMinWidth = 120.0;
    const double kMinCompartmentHeight = 10.0; // empty attribute/operation section

    char visibilitySymbol(Visibility v) {
        switch (v) {
        case Visibility::Public:    return '+';
        case Visibility::Private:   return '-';
        case Visibility::Protected: return '#';
        case Visibility::Package:   return '~';
        }
        return '-';
    }

    std::string attributeLine(const Attribute& a) {
        std::string line;
        line += visibilitySymbol(a.visibility);
        line += " " + a.name;
        if (!a.type.empty()) line += ": " + a.type;
        return line;
    }

    std::string operationLine(const Operation& o) {
        std::string line;
        line += visibilitySymbol(o.visibility);
        line += " " + o.name + "()";
        if (!o.returnType.empty()) line += ": " + o.returnType;
        return line;
    }
}

void computeNodeSizes(ClassDiagram& diagram) {
    for (auto& cls : diagram.classes) {
        double longestLineChars = static_cast<double>(cls.name.size());

        for (const auto& a : cls.attributes)
            longestLineChars = std::max(longestLineChars, static_cast<double>(attributeLine(a).size()));

        for (const auto& o : cls.operations)
            longestLineChars = std::max(longestLineChars, static_cast<double>(operationLine(o).size()));

        cls.width = std::max(kMinWidth, longestLineChars * kCharWidth + 2 * kPaddingX);

        double nameHeight = kLineHeight + 2 * kPaddingY;

        double attrHeight = cls.attributes.empty()
            ? kMinCompartmentHeight
            : cls.attributes.size() * kLineHeight + 2 * kPaddingY;

        double opHeight = cls.operations.empty()
            ? kMinCompartmentHeight
            : cls.operations.size() * kLineHeight + 2 * kPaddingY;

        cls.height = nameHeight + attrHeight + opHeight;
    }
}