//
// Created by alex on 7/15/15.
//

#include "SvgParsers.h"

#include "node_transform.hpp"
#include "tagdparser.h"
#include "tess_svg/GlDefs.h"
#include "util_helpers.h"

#include <math.h> //NOLINT
#include <pugixml.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

extern float BEIZER_FLATNESS;
extern std::size_t ELLIPSE_POINTS;

namespace {
void throwIfWrongTag(const std::string &expected, const pugi::xml_node &node)
{
    if (toLower(node.name()) != expected)
    {
        throw std::runtime_error("Invalid node tag: expected " + expected + " but got "
                                 + toLower(node.name()));
    }
}

void generateEllipsePoints(Vertexes &pts, float cx, float cy, float rx, float ry)
{
    if (rx <= 0 || ry <= 0)
    {
        return;
    }

    const int segments = static_cast<int>(std::max(static_cast<size_t>(1), ELLIPSE_POINTS));
    for (int i = 0; i < segments; ++i)
    {
        const double theta = 2.0 * M_PI * i / segments;
        pts.emplace_back(cx + rx * std::cos(theta), cy + ry * std::sin(theta));
    }
}

Loops pointsToLoops(std::vector<GlVertex> pts, const GlVertex::trans_matrix_t &finalTransform)
{
    for (auto &p : pts)
    {
        p.translate(finalTransform);
    }
    return {std::move(pts)};
}

const auto &nodesRegistry()
{
    // Important! Keep new node names in LOWER case.
    static const std::map<std::string, ShapeParser> reg = {
      {"path", SvgParsers::parsePath},        {"rect", SvgParsers::parseRect},
      {"circle", SvgParsers::parseCircle},    {"ellipse", SvgParsers::parseEllipse},
      {"line", SvgParsers::parseLine},        {"polygon", SvgParsers::parsePolygon},
      {"polyline", SvgParsers::parsePolygon},
    };
    return reg;
}

ShapeParser findParser(const std::string &lowerCasedNodeName)
{
    auto &reg = nodesRegistry();
    const auto it = reg.find(lowerCasedNodeName);
    if (it != reg.end())
    {
        return it->second;
    }
    return nullptr;
}

} // namespace

NodeParser::NodeParser(const pugi::xml_node &node) :
    node(node),
    nodeName_(toLower(node.name())),
    parser(findParser(nodeName_))
{
}

bool NodeParser::isSupported() const
{
    return static_cast<bool>(parser);
}

const std::string &NodeParser::nodeName() const
{
    return nodeName_;
}

Loops NodeParser::parse(const GlVertex::trans_matrix_t &parentTransform) const
{
    if (!parser)
    {
        throw std::runtime_error("Node " + nodeName_ + " is not supported yet.");
    }
    return parser(node, parentTransform);
}

namespace SvgParsers {

// --- PATH ---
Loops parsePath(const pugi::xml_node &node, GlVertex::trans_matrix_t parentTransform)
{
    throwIfWrongTag("path", node);
    // Note, we took parameter as copy, so it is local copy change.
    updateTransform(node, parentTransform);
    return TagDParser::split(node.attribute("d").as_string(), parentTransform);
}

// --- RECT ---
Loops parseRect(const pugi::xml_node &node, GlVertex::trans_matrix_t parentTransform)
{
    throwIfWrongTag("rect", node);
    updateTransform(node, parentTransform);

    const auto x = node.attribute("x").as_float();
    const auto y = node.attribute("y").as_float();
    const auto w = node.attribute("width").as_float();
    const auto h = node.attribute("height").as_float();

    std::vector<GlVertex> pts = {{x, y}, {x + w, y}, {x + w, y + h}, {x, y + h}};
    return pointsToLoops(std::move(pts), parentTransform);
}

// --- CIRCLE ---
Loops parseCircle(const pugi::xml_node &node, GlVertex::trans_matrix_t parentTransform)
{
    throwIfWrongTag("circle", node);
    updateTransform(node, parentTransform);

    const auto cx = node.attribute("cx").as_float();
    const auto cy = node.attribute("cy").as_float();
    const auto r = node.attribute("r").as_float();

    Vertexes pts;
    generateEllipsePoints(pts, cx, cy, r, r); // Для круга rx = ry
    return pointsToLoops(std::move(pts), parentTransform);
}

// --- ELLIPSE ---
Loops parseEllipse(const pugi::xml_node &node, GlVertex::trans_matrix_t parentTransform)
{
    throwIfWrongTag("ellipse", node);
    updateTransform(node, parentTransform);

    const auto cx = node.attribute("cx").as_float();
    const auto cy = node.attribute("cy").as_float();
    const auto rx = node.attribute("rx").as_float();
    const auto ry = node.attribute("ry").as_float();

    Vertexes pts;
    generateEllipsePoints(pts, cx, cy, rx, ry);
    return pointsToLoops(std::move(pts), parentTransform);
}

// --- LINE ---
Loops parseLine(const pugi::xml_node &node, GlVertex::trans_matrix_t parentTransform)
{
    throwIfWrongTag("line", node);
    updateTransform(node, parentTransform);

    const auto x1 = node.attribute("x1").as_float();
    const auto y1 = node.attribute("y1").as_float();
    const auto x2 = node.attribute("x2").as_float();
    const auto y2 = node.attribute("y2").as_float();

    return pointsToLoops({{x1, y1}, {x2, y2}}, parentTransform);
}

// --- POLYGON / POLYLINE ---
Loops parsePolygon(const pugi::xml_node &node, GlVertex::trans_matrix_t parentTransform)
{
    const std::string name = toLower(node.name());
    const bool is_polygon = (name == "polygon");
    if (!is_polygon && name != "polyline")
    {
        throwIfWrongTag("polygon", node);
    }
    updateTransform(node, parentTransform);
    auto loops = TagDParser::split(node.attribute("points").as_string(), parentTransform);
    if (!loops.empty() && is_polygon)
    {
        auto &pts = loops.front();
        if (pts.size() > 2)
        {
            const GlVertex &first = pts.front();
            const GlVertex &last = pts.back();
            if (std::abs(first.x() - last.x()) > 1e-5 || std::abs(first.y() - last.y()) > 1e-5)
            {
                pts.push_back(first);
            }
        }
    }
    return loops;
}

} // namespace SvgParsers
