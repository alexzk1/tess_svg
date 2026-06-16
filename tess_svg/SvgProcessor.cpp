//
// Created by alex on 7/15/15.
//

#include "SvgProcessor.h"

#include "tess_svg/GlDefs.h"
#include "tess_svg/SvgParsers.h"
#include "tess_svg/geometry_engine.hpp"
#include "tess_svg/node_transform.hpp"
#include "tess_svg/processing_data.hpp"
#include "tess_svg/util_helpers.h"

#include <cstddef>
#include <format>
#include <iostream>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace {

struct RecursionParameters
{
    GlVertex::trans_matrix_t parentTrans = GlVertex::getIdentity();
    Loops singleNodeLoops{};
};

void parseSvgWorld(SvgWorld &output, std::size_t recursionLevel, const pugi::xml_node &node,
                   RecursionParameters &params)
{
    const NodeParser parser(node);

    if (parser.isSupported())
    {
        // Bottom of the recursion - single node which cannot have children.
        if (0u == recursionLevel) [[unlikely]]
        {
            throw std::runtime_error("SVG is broken. Root object must be <svg>.");
        }
        params.singleNodeLoops = parser.parse(params.parentTrans);
    }
    else
    {
        const bool groupped = (parser.nodeName() == "g");
        const std::string parentId(node.attribute("id").as_string());
        if (groupped)
        {
            // Result will have group with multiply results.
            output.scene.emplace_back(SvgGroup{parentId, {}});
        }

        RecursionParameters childrenParams{params.parentTrans};
        updateTransform(node, childrenParams.parentTrans);

        bool hadDef = false; // strings optimization
        for (pugi::xml_node childNode = node.first_child(); childNode;
             childNode = childNode.next_sibling())
        {
            // Note, it will try to parse <defs><defs></defs></defs> too, however nested one is
            // invalid and will be lost any way.
            if (!hadDef && NodeParser::nodeName(childNode) == "defs") [[unlikely]]
            {
                // 1 <defs> per 1 <svg> is expected.
                hadDef = true;
                SvgWorld tmp;
                RecursionParameters defsRecursion{};
                // Passing <defs> node itself as it would be <svg>.
                parseSvgWorld(tmp, 0u, childNode, defsRecursion);
                // Treating <defs> as new <svg> in terms of geometry elements.
                output.defs = std::move(tmp.scene);
                continue;
            }
            childrenParams.singleNodeLoops.clear();
            parseSvgWorld(output, recursionLevel + 1, childNode, childrenParams);

            if (childrenParams.singleNodeLoops.size() > 0)
            {
                ParsedSvgElement tess;
                tess.setAttributes(childNode);
                if (!groupped)
                {
                    // <svg><rect/></svg> case.
                    // Result will have 1 "group" which will have 1 element.
                    output.scene.emplace_back(SvgGroup{tess.id(), {}});
                }

                tess.data = std::move(childrenParams.singleNodeLoops);
                output.scene.back().elements.emplace_back(std::move(tess));
            }
        }
    }
}

// Collapses SvgGroups into collection of the Loops (used witn Union).
std::vector<Loops> collapseGroups(const std::vector<SvgGroup> &groups)
{
    if (groups.empty())
    {
        return {};
    }
    std::vector<Loops> res;
    res.reserve(groups.size());

    for (const auto &group : groups)
    {
        auto total = getLoopsFromGroup(group);
        res.insert(res.end(), std::make_move_iterator(total.begin()),
                   std::make_move_iterator(total.end()));
    }
    return res;
}

/**
 * @brief Reconstructs a hierarchical structure from flattened topological islands.
 * Each island (set of loops) is treated as a distinct physical entity.
 *
 * @param islands A vector of 'Loops', where each entry is an independent island (outer + holes).
 * @return std::vector<SvgGroup> A collection of groups, each containing one element.
 */
std::vector<SvgGroup> restoreGroups(std::vector<Loops> islands)
{
    if (islands.empty())
    {
        return {};
    }

    std::vector<SvgGroup> groups;
    groups.reserve(islands.size());

    for (size_t i = 0; i < islands.size(); ++i)
    {
        // 1. Create the element representing this specific island
        ParsedSvgElement element;
        element.data = std::move(islands[i]); // Type: Loops

        // Generate unique ID for the element
        element.attributes["id"] = std::format("entity_{}", i);

        // 2. Create a group to wrap this element (maintaining hierarchy integrity)
        SvgGroup group;
        group.id_ = std::format("grp_{}", i);
        group.elements.push_back(std::move(element));

        groups.push_back(std::move(group));
    }

    return groups;
}

} // namespace

SvgWorld loadSvgWorld(std::istream &src)
{
    SvgWorld world;
    pugi::xml_document doc;

    auto svgTree = doc.load(src);
    if (svgTree.status != pugi::status_ok)
    {
        std::cerr << "Error parsing file, status: " << svgTree.status << ": "
                  << svgTree.description() << std::endl;
        throw xmlerror(svgTree.description());
    }

    RecursionParameters initialParams{};
    parseSvgWorld(world, 0u, doc.first_child(), initialParams);

    return world;
}

SvgWorldTransformers &SvgWorldTransformers::addTransformer(SvgWorldTransformer trans)
{
    transformers.emplace_back(std::move(trans));
    return *this;
}

SvgWorld SvgWorldTransformers::buildSurroundingPolygons(SvgWorld world)
{
    for (const auto &trans : transformers)
    {
        if (trans)
        {
            trans(world);
        }
    }

    finalizeGroupsContours(world.scene);
    world.defs.clear(); // Note, it is a copy!
    return world;
}

void unionElementsTransformer(SvgWorld &world)
{
    auto loops = collapseGroups(world.scene);
    loops = geometry_engine::unionShapes(loops);
    world.scene = restoreGroups(std::move(loops));
}

// void clipByDefsTransformer(SvgWorld &world)
// {
//     // Мы не меняем defs (сохраняем их для других элементов),
//     // работаем только со сценой.
//     for (auto &group : world.scene)
//     {
//         for (auto &element : group.elements)
//         {
//             if (element.isEmpty())
//                 continue;

//             // Ищем атрибут clip-path="url(#ID)"
//             const auto it = element.attributes.find("clip-path");
//             if (it != element.attributes.end() && it->second.find("url(#") != std::string::npos)
//             {
//                 std::string id =
//                   extractIdFromUrl(it->second); // Функция парсинга "url(#id)" -> "id"

//                 // 1. Получаем маску (уже объединенную, если это группа)
//                 Loops mask_loops = getUnifiedDefinition(world.defs, id);

//                 if (!mask_loops.empty())
//                 {
//                     // 2. Применяем Intersection к геометрии элемента
//                     auto &current_data = element.data;
//                     if (std::holds_alternative<Loops>(current_data))
//                     {
//                         Loops subject = std::get<Loops>(current_data);
//                         Loops result = geometry_engine::intersectWithMasks(subject, mask_loops);
//                         current_data = std::move(result);
//                     }
//                 }
//             }
//         }
//     }
// }

std::optional<Loops> getLoopsFromElement(const ParsedSvgElement &element)
{
    if (auto *p = std::get_if<Loops>(&element.data))
    {
        // 1. Проверяем атрибут fill-rule для этого конкретного элемента
        const auto it = element.attributes.find("fill-rule");
        const bool isEvenOdd = (it != element.attributes.end() && toLower(it->second) == "evenodd");
        if (isEvenOdd)
        {
            // 2. Если это EvenOdd, мы ОБЯЗАТЕЛЬНО разрешаем его внутреннюю топологию прямо
            // сейчас. Эта функция превратит набор контуров с разным направлением в
            // корректно ориентированные "острова и дырки".
            return geometry_engine::resolveEvenOddInternalTopology(*p);
        }
        else
        {
            // 3. Если правило другое (например, NonZero по умолчанию),
            // просто добавляем контуры как есть.
            return *p;
        }
    }
    return std::nullopt;
}

std::vector<Loops> getLoopsFromGroup(const SvgGroup &group)
{
    std::vector<Loops> all_elements_loops;
    for (const auto &element : group.elements)
    {
        if (auto opt = getLoopsFromElement(element))
        {
            all_elements_loops.emplace_back(std::move(*opt));
        }
        // Do not throw, as it could be gradient, for example. Just pass.
    }
    return all_elements_loops;
}
