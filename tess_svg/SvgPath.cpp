//
// Created by alex on 7/15/15.
//

#include "SvgPath.h"
#include "tagdparser.h"
#include "Tesselate.h"


const SvgPath::transform_funcs SvgPath::transforms = {
        SvgPath::func_holder("translate(", [](auto& m, auto& b)
        {
            std::sregex_token_iterator end;
            auto x = std::stod(*b++);
            auto y = ((end == b) ? 0: std::stod(*b));
            m(0,2) = x;
            m(1,2) = y;
        }),

        SvgPath::func_holder("matrix(", [](auto& m, auto& v2)
        {
            used_vectors_type vals[] = {
                    std::stod(*v2++),
                    std::stod(*v2++),
                    std::stod(*v2++),
                    std::stod(*v2++),
                    std::stod(*v2++),
                    std::stod(*v2++)
            };
            for (auto j = 0; j < 3; j++)
            {
                m(0,j) = vals[j*2 + 0];
                m(1,j) = vals[j*2 + 1];
            }

        }),

        SvgPath::func_holder("scale(", [](auto& m, auto& b)
        {
            std::sregex_token_iterator end;
            auto x = std::stod(*b++);
            auto y = (end == b) ? x : std::stod(*b);

            m = GlVertex::getZeroMatrix();
            m(0,0) = x;
            m(1,0) = y;
        }),
};

SvgPath::SvgPath(const pugi::xml_node& node, const pugi::xml_node& parentNode)
        :parentNode(parentNode)
{
    parse_node(node);
}

void SvgPath::parse_node(const pugi::xml_node &node)
{
    Tesselate tsl;


    GlVertex::trans_matrix_t vtr = GlVertex::getIdentity();

    loops.clear();

    auto trans = std::string(parentNode.attribute("transform").as_string("translate(0,0)")); //std::tolower(std::string(parentNode.attribute("transform").as_string("translate(0,0)")),std::locale::classic());

    for (auto& f : transforms)
        if (f.exec(trans, vtr))
            break;

    if ( std::string(node.name()) == "path")
    {
        loops = TagDParser::split(node.attribute("d").as_string(), vtr);
    }
}

const Loops &SvgPath::getLoops() const
{
    return loops;
}

bool SvgPath::func_holder::exec(const std::string& src, GlVertex::trans_matrix_t& res) const
{
    size_t pos = src.find(mask);
    if (pos!=std::string::npos)
    {
        std::regex rgx("\\s+|\\,+");
        pos += mask.length();
        std::sregex_token_iterator iter(src.begin()+pos,
                                        src.end(),
                                        rgx,-1);
        func(res, iter);
        return true;
    }
    return false;
}