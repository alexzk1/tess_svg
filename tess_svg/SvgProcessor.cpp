//
// Created by alex on 7/15/15.
//

#include "SvgProcessor.h"

#include "tess_svg/GlDefs.h"
#include "tess_svg/SvgParsers.h"
#include "tess_svg/node_transform.hpp"
#include "tess_svg/processing_data.hpp"

#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

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

        for (pugi::xml_node childNode = node.first_child(); childNode;
             childNode = childNode.next_sibling())
        {
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

SvgWorldTransformers &SvgWorldTransformers::addTransformer()
{
    // TODO:

    return *this;
}

SvgWorld SvgWorldTransformers::buildSurroundingPolygons(SvgWorld world)
{
    // TODO: Apply added transformers
    finalizeGroupsContours(world.scene);
    world.defs.clear(); // Note, it is a copy!
    return world;
}
