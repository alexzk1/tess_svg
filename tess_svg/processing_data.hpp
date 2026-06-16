#pragma once

#include "GlDefs.h"
#include "Tesselate.h"
#include "util_helpers.h"

#include <pugixml.hpp>

#include <map>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

/// @brief Prepared SVG. Each element is converted into polygon and translated into World space.
/// It is produced by SvgProcessor.
struct ParsedSvgElement
{
    /// @brief Current data present in this object. Data changes when object flows through
    /// processing chain.
    using GeometricalData = std::variant<std::monostate, Loops, Polyline>;

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

    [[nodiscard]]
    bool isEmpty() const noexcept
    {
        return std::holds_alternative<std::monostate>(data);
    }

    /// @brief Checks if it is final resul data i.e. it is bounding polygon in World coordinates.
    [[nodiscard]]
    bool isFinal() const noexcept
    {
        return std::holds_alternative<Polyline>(data);
    }

    /// @brief Access to final data if any.
    /// @returns Reference to final data if any or to empty data container otherwise.
    [[nodiscard]]
    const Polyline &finalData() const noexcept
    {
        static const Polyline kNothing;
        if (const auto *p = std::get_if<Polyline>(&data))
        {
            return *p;
        }
        return kNothing;
    }

    /// @brief name=value attributes from original element (`rect`, `path` etc.) in SVG.
    std::map<std::string, std::string> attributes;

    /// @brief Current geometrical data of this object.
    GeometricalData data{std::monostate{}};
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

    /// @brief Clones all fields except elements.
    [[nodiscard]]
    SvgGroup cloneNoElements() const
    {
        return {id_, {}};
    }

    std::string id_;
    ParsedSvgElements elements;
};

/// @brief All SVG groups (single root element becomes 1 group with 1 element) and geometrical data
/// from <defs>.
struct SvgWorld
{
    /// @brief Scene geometrical objects translated to world space or final world space contour(s).
    std::vector<SvgGroup> scene;
    /// @brief <defs> geometrical objects.
    std::vector<SvgGroup> defs;

    void reset()
    {
        scene.clear();
        defs.clear();
    }
};

/// @brief Makes final transformation of the groups and produces bounding polygon(s) in World
/// coordinates.
/// @note 1 SVG usually represents 1 "world object", however prev steps could decompose single SVG
/// into multiply geometrical sub-objects. Each of them will be passed to Tesselate::process() and
/// produce own output value. Join of all outputs equals to perimeter of original SVG.
inline void finalizeGroupsContours(std::vector<SvgGroup> &groups)
{
    for (SvgGroup &g : groups)
    {
        for (auto &elem : g.elements)
        {
            if (auto *pd = std::get_if<Loops>(&elem.data))
            {
                Tesselate ts;
                elem.data = ts.process(*pd, TessMode::Contours);
                continue;
            }
            throw std::runtime_error("Invalid input to finilazer.");
        }
    }
}
