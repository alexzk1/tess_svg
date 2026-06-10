//
// Created by alex on 7/15/15.
//

#include "SvgProcessor.h"

#include "tess_svg/GlDefs.h"
#include "tess_svg/SvgParsers.h"
#include "tess_svg/node_transform.hpp"

#include <cstddef>
#include <functional>
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

void SvgProcessor::postProcessTesselatedVerteces(const std::function<void(BoundedGroup &)> &func)
{
    for (auto &v : tesselated)
    {
        func(v.second);
        v.second.makeBounds();
    }
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
        const std::string id(node.attribute("id").as_string());
        if (groupped)
        {
            tesselated.emplace_back(std::make_pair(id, BoundedGroup()));
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
                const auto childNodeId = childNode.attribute("id").as_string();
                if (!groupped)
                {
                    // <svg><rect/></svg> case.
                    tesselated.emplace_back(std::make_pair(childNodeId, BoundedGroup()));
                }

                TessResult tess;
                tess.setAttributes(childNode);
                tess.vertexes = ts.process(childrenParams.singleNodeLoops, true);
                tesselated.back().second.pathes.emplace_back(std::make_pair(childNodeId, tess));

                if (!groupped)
                {
                    // <svg><rect/></svg> case.
                    tesselated.back().second.vertexes = tess.vertexes;
                    tesselated.back().second.makeBounds();
                }
            }
        }

        if (groupped)
        {
            // 1. Сначала заставляем всех детей посчитать свои границы
            for (auto &p : tesselated.back().second.pathes)
            {
                p.second.makeBounds();
            }

            // 2. Считаем общие границы группы на основе границ детей
            auto &currentGroup = tesselated.back().second;
            currentGroup.bounds.reset();

            for (auto &p : currentGroup.pathes)
            {
                // Добавляем углы границ ребенка в границы группы
                currentGroup.bounds.add_point(p.second.bounds.xmin, p.second.bounds.ymin);
                currentGroup.bounds.add_point(p.second.bounds.xmax, p.second.bounds.ymax);
            }
        }
    }
}

SvgProcessor::WithBounds::~WithBounds() = default;
