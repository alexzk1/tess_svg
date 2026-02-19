//
// Created by alex on 7/15/15.
//

#include "SvgProcessor.h"

#include "util_helpers.h"

#include <iostream>

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

    parse(doc.first_child(), doc.first_child(), nullptr, nullptr);
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

void SvgProcessor::parse(const pugi::xml_node &node, const pugi::xml_node &parent, Loops *loops,
                         Loops *total_loops_param)
{
    // todo: add more text nodes to tesselated

    const std::string node_name(toLower(node.name()));

    if (node_name == "path")
    {
        SvgPath ptr(node, parent);
        auto &a = ptr.getLoops();
        if (loops != nullptr)
        {
            loops->clear();
            *loops = a;
        }

        if (total_loops_param != nullptr)
            total_loops_param->insert(total_loops_param->end(), a.cbegin(), a.cend());
    }
    else
    {
        Loops total_loops;
        const bool groupped = (node_name == "g");
        std::string id(node.attribute("id").as_string());
        if (groupped)
            tesselated.emplace_back(std::make_pair(id, BoundedGroup()));

        for (pugi::xml_node tool = node.first_child(); tool; tool = tool.next_sibling())
        {
            const std::string tool_id(tool.attribute("id").as_string());
            Loops curr_loops;

            parse(tool, node, &curr_loops, (groupped) ? &total_loops : nullptr);

            if (curr_loops.size() > 0)
            {
                if (!groupped)
                    tesselated.emplace_back(std::make_pair(tool_id, BoundedGroup()));

                TessResult tess;
                tess.setAttributes(tool);
                tess.vertexes = ts.process(curr_loops, true);
                auto path = std::make_pair(tool_id, tess);
                tesselated.back().second.pathes.push_back(path);
                if (!groupped)
                {
                    tesselated.back().second.vertexes = tess.vertexes;
                    tesselated.back().second.makeBounds();
                }
            }
        }

        if (groupped)
        {
            tesselated.back().second.makeBounds();
            if (total_loops.size() > 0)
            {
                tesselated.back().second.vertexes = ts.process(total_loops, true);
                tesselated.back().second.makeBounds();
            }
        }
    }
}
