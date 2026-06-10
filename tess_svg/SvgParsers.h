//
// Created by alex on 7/15/15.
//

#pragma once

#include "GlDefs.h"

#include <pugixml.hpp>

#include <functional>
#include <string>

/// @brief Node element parser prototype.
/// @param nodeToParse source node we try to parse.
/// @param parentTransform parsed combined transformation of the parent node.
using ShapeParser = std::function<Loops(const pugi::xml_node & /*nodeToParse*/,
                                        GlVertex::trans_matrix_t /*parentTransform*/)>;

/// @brief Parsers of the SVG elements.
namespace SvgParsers {
/// @brief Parse `path` @p node using @p parentTransform (if any).
/// @return Loops which is source data for tesselator.
/// @throws If node does not match expected type.
Loops parsePath(const pugi::xml_node &node, GlVertex::trans_matrix_t parentTransform);

/// @brief Parse `rect` @p node using @p parentTransform (if any).
/// @return Loops which is source data for tesselator.
/// @throws If node does not match expected type.
Loops parseRect(const pugi::xml_node &node, GlVertex::trans_matrix_t parentTransform);

/// @brief Parse `circle` @p node using @p parentTransform (if any).
/// @return Loops which is source data for tesselator.
/// @throws If node does not match expected type.
Loops parseCircle(const pugi::xml_node &node, GlVertex::trans_matrix_t parentTransform);

/// @brief Parse `ellipse` @p node using @p parentTransform (if any).
/// @return Loops which is source data for tesselator.
/// @throws If node does not match expected type.
Loops parseEllipse(const pugi::xml_node &node, GlVertex::trans_matrix_t parentTransform);

/// @brief Parse `line` @p node using @p parentTransform (if any).
/// @return Loops which is source data for tesselator.
/// @throws If node does not match expected type.
Loops parseLine(const pugi::xml_node &node, GlVertex::trans_matrix_t parentTransform);

/// @brief Parse `polygon` and/or `polyline` @p node using @p parentTransform (if any).
/// @return Loops which is source data for tesselator.
/// @throws If node does not match expected type.
/// @note Current class Tesselate will treat node as closed polyline no matter of input.
Loops parsePolygon(const pugi::xml_node &node, GlVertex::trans_matrix_t parentTransform);
} // namespace SvgParsers

/// @brief Tries to find valid node parser and allows to apply it to node.
class NodeParser
{
  public:
    explicit NodeParser(const pugi::xml_node &node);

    /// @brief Checks if this node type is supported.
    /// @return true if this node is supported by parsers.
    [[nodiscard]]
    bool isSupported() const;

    /// @brief Provides access to XML/Svg node name for caching and proper case purposes.
    /// @return Reference to lowercased XML/Svg node name.
    [[nodiscard]]
    const std::string &nodeName() const;

    /// @brief Applies parser to the node with @p parentTransform.
    /// @return Loops for tesselator.
    /// @throws if this->isSupported() would return false.
    [[nodiscard]]
    Loops parse(const GlVertex::trans_matrix_t &parentTransform) const;

  protected:
    const pugi::xml_node &node;
    std::string nodeName_;
    ShapeParser parser;
};
