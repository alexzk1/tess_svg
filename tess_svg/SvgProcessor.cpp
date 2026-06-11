//
// Created by alex on 7/15/15.
//

#include "SvgProcessor.h"

#include "tess_svg/GlDefs.h"
#include "tess_svg/SvgParsers.h"
#include "tess_svg/node_transform.hpp"

#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

struct SvgProcessor::RecursionParameters
{
    GlVertex::trans_matrix_t parentTrans = GlVertex::getIdentity();
    Loops singleNodeLoops{};
};

SvgProcessor::SvgProcessor(std::istream &src) :
    doc()
{
    parse_svg_file(src);
}

void SvgProcessor::parse_svg_file(std::istream &src)
{
    using namespace pugi;
    using namespace std;
    tesselated.clear();

    auto res = doc.load(src);
    if (res.status != status_ok)
    {
        cerr << "Error parsing file, status: " << res.status << ": " << res.description() << endl;
        throw xmlerror(res.description());
    }

    RecursionParameters initialParams{};
    parse(0u, doc.first_child(), initialParams);
}

const SvgProcessor::group_t &SvgProcessor::getTesselated() const
{
    return tesselated;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void SvgProcessor::parse(std::size_t recursionLevel, const pugi::xml_node &node,
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
            tesselated.emplace_back(std::make_pair(parentId, BoundedGroup()));
        }

        RecursionParameters childrenParams{params.parentTrans};
        updateTransform(node, childrenParams.parentTrans);

        for (pugi::xml_node childNode = node.first_child(); childNode;
             childNode = childNode.next_sibling())
        {
            childrenParams.singleNodeLoops.clear();
            parse(recursionLevel + 1, childNode, childrenParams);

            if (childrenParams.singleNodeLoops.size() > 0)
            {
                std::string childNodeId = childNode.attribute("id").as_string();
                if (childNodeId.empty())
                {
                    childNodeId = childNode.name();
                }
                if (!groupped)
                {
                    // <svg><rect/></svg> case.
                    // Result will have 1 "group" which will have 1 element.
                    tesselated.emplace_back(std::make_pair(childNodeId, BoundedGroup()));
                }

                TessResult tess;
                tess.setAttributes(childNode);
                tess.vertexes = ts.process(childrenParams.singleNodeLoops, true);
                tesselated.back().second.pathes.emplace_back(
                  std::make_pair(childNodeId, std::move(tess)));
            }
        }
    }
}
