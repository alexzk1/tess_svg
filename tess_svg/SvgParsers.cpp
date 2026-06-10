//
// Created by alex on 7/15/15.
//

#include "SvgParsers.h"

#include "node_transform.hpp"
#include "tagdparser.h"
#include "tess_svg/GlDefs.h"
#include "util_helpers.h"

#include <pugixml.hpp>

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
SvgPath::SvgPath(const pugi::xml_node &node, const pugi::xml_node &parentNode) :
    parentNode(parentNode)
{
    parse_node(node);
}

void SvgPath::parse_node(const pugi::xml_node &node)
{
    GlVertex::trans_matrix_t vtr = GlVertex::getIdentity();

    loops.clear();
    // 1. Сначала применяем трансформ родителя
    updateTransform(parentNode, vtr);
    // 2. Затем ПОВЕРХ применяем трансформ самой ноды (умножение произойдет внутри)
    updateTransform(node, vtr);

    if (toLower(node.name()) == "path")
    {
        loops = TagDParser::split(node.attribute("d").as_string(), vtr);
    }
}

const Loops &SvgPath::getLoops() const
{
    return loops;
}
