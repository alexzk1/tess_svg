//
// Created by alex on 7/15/15.
//

#include "SvgParsers.h"

#include "node_transform.hpp"
#include "tagdparser.h"
#include "tess_svg/GlDefs.h"
#include "util_helpers.h"

#include <pugixml.hpp>

#include <stdexcept>
#include <string>

namespace {
void throwIfWrongTag(const std::string &name, const pugi::xml_node &node)
{
    if (toLower(node.name()) != name)
    {
        throw std::runtime_error("Invalid node was passed to handler function.");
    }
}
} // namespace

namespace SvgParsers {
Loops parsePath(const pugi::xml_node &node, GlVertex::trans_matrix_t parentTransform)
{
    throwIfWrongTag("path", node);
    updateTransform(node, parentTransform);
    return TagDParser::split(node.attribute("d").as_string(), parentTransform);
}
} // namespace SvgParsers
