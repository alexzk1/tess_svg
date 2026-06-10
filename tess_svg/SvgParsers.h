//
// Created by alex on 7/15/15.
//

#pragma once

#include "GlDefs.h"

#include <pugixml.hpp>

#include <functional>

using ShapeParser = std::function<Loops(const pugi::xml_node & /*nodeToParse*/,
                                        GlVertex::trans_matrix_t /*parentTransform*/)>;

namespace SvgParsers {

Loops parsePath(const pugi::xml_node &node, GlVertex::trans_matrix_t parentTransform);
Loops parseRect(const pugi::xml_node &node, GlVertex::trans_matrix_t parentTransform);
Loops parseCircle(const pugi::xml_node &node, GlVertex::trans_matrix_t parentTransform);
Loops parseEllipse(const pugi::xml_node &node, GlVertex::trans_matrix_t parentTransform);
Loops parseLine(const pugi::xml_node &node, GlVertex::trans_matrix_t parentTransform);
Loops parsePolygon(const pugi::xml_node &node, GlVertex::trans_matrix_t parentTransform);
} // namespace SvgParsers
