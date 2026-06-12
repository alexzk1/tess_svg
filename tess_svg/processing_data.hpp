#pragma once

#include "GlDefs.h"
#include "util_helpers.h"

#include <pugixml.hpp>

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

/// @brief Prepared SVG. Each element is converted into polygon and translated into World space.
/// It is produced by SvgProcessor.
struct ParsedSvgElement
{
    /// @brief Attributes setter.
    /// Ensures all attribute names are lowercased.
    /// If it is missing ID then sets it to element name.
    void setAttributes(const pugi::xml_node &node)
    {
        attributes.clear();

        for (auto attr = node.attributes_begin(); attr != node.attributes_end(); attr++)
        {
            attributes[std::string(toLower(attr->name()))] = std::string(attr->as_string());
        }
        const auto it = attributes.find("id");
        if (it == attributes.end())
        {
            attributes["id"] = node.name();
        }
    }

    /// @brief Given ID of the SVG element if any.
    /// @returns Original `id` of the SVG element or it's name.
    /// @throws If attribut ID was not set.
    [[nodiscard]]
    const std::string &id() const
    {
        const auto it = attributes.find("id");
        if (it != attributes.end()) [[likely]]
        {
            return it->second;
        }
        throw std::runtime_error("Attribute `id` was not found.");
    }

    /// @brief name=value attributes from original element (`rect`, `path` etc.) in SVG.
    std::map<std::string, std::string> attributes;

    /// @brief Final result representing bounding polygon of the element.
    Vertexes vertexes;
};

using ParsedSvgElements = std::vector<ParsedSvgElement>;

/// @brief Represents named SVG group.
struct SvgGroup
{
    [[nodiscard]]
    const std::string &id() const
    {
        return id_;
    }

    std::string id_;
    ParsedSvgElements elements;
};

using SvgGroups = std::vector<SvgGroup>;
