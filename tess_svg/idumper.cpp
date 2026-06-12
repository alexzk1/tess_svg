#include "idumper.h"

#include "tess_svg/GlDefs.h"
#include "tess_svg/SvgProcessor.h"
#include "tess_svg/processing_data.hpp"

#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <ostream>
#include <string>
#include <utility>

namespace {
[[nodiscard]]
std::string fixClassName(std::string cname, bool force_upper_first)
{
    cname.erase(std::remove_if(cname.begin(), cname.end(),
                               [](auto c) -> bool {
                                   const bool r = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
                                                  || (c >= '0' && c <= '9') || c == '_';
                                   return !r;
                               }),
                cname.end());

    if (!cname.empty())
    {
        if (!std::isalpha(cname[0]))
        {
            cname = "C" + cname;
        }

        if (force_upper_first && !cname.empty())
        {
            const auto first = static_cast<unsigned char>(cname[0]);
            // UTF-8 check.
            if (first < 128)
            {
                cname[0] = static_cast<char>(std::toupper(first));
            }
        }
    }
    return cname;
}
} // namespace

IDumper::IDumper(std::ostream &out, const SvgProcessor &, std::string namePrefix) :
    outstr(out),
    namePrefix(std::move(namePrefix))
{
}

IDumper::~IDumper() = default;

//************************************************************************************************************************************************************
void JsonDumper::dumpPath(const SvgGroups &what) const
{
    using namespace std;
    using namespace nlohmann;

    const auto out_vertex = [](const Vertexes &what) -> json {
        json vertex = json::array();
        for (const auto &v : what)
        {
            vertex.push_back(v.x());
            vertex.push_back(v.y());
        }
        return vertex;
    };

    const auto out_polygon = [&out_vertex](const auto &g) {
        json poly = json::object();
        poly["vertexes"] = out_vertex(g.finalData());
        return poly;
    };

    json object = json::object();

    for (const auto &g : what)
    {
        if (g.elements.empty())
        {
            continue;
        }

        if (g.elements.size() > 1)
        {
            json group = json::object();
            json cont = json::object();
            for (const auto &p : g.elements)
            {
                cont[p.id()] = out_polygon(p);
            }
            group["contains"] = cont;
            object[g.id()] = group;
        }
        else
        {
            const auto &p = g.elements.at(0);
            object[p.id()] = out_polygon(p);
        }
    }

    json obj2 = json::object();

    if (!namePrefix.empty())
    {
        obj2[namePrefix] = object;
    }

    const auto &oj = (namePrefix.empty()) ? object : obj2;
    if (pretty)
    {
        outstr << std::setw(4) << oj;
    }
    else
    {
        outstr << oj;
    }
}

void JsonDumper::dumpVertexes(const Vertexes &) const
{
}

JsonDumper::JsonDumper(std::ostream &out, const SvgProcessor &pr, const std::string &namePrefix,
                       bool pretty) :
    IDumper(out, pr, namePrefix),
    pretty(pretty)
{
    dumpPath(pr.getTesselated());
}

//************************************************************************************************************************************************************
void LuaDumper::dumpPath(const SvgGroups &what) const
{
    auto cname = fixClassName(namePrefix, false);
    if (use_local)
    {
        outstr << "local ";
    }
    outstr << ((cname.empty()) ? "figure_default" : cname) << " = {" << std::endl;
    for (const auto &g : what)
    {

        const auto name = "group_" + g.id();
        outstr << name << " = {" << std::endl;

        for (const auto &tess_result : g.elements)
        {
            outstr << tess_result.id() << " = { " << std::endl << "vertexes = {";
            dumpVertexes(tess_result.finalData());
            outstr << "}," << std::endl;
        }
        outstr << "}," << std::endl;
    }
    outstr << "}" << std::endl;
}

void LuaDumper::dumpVertexes(const Vertexes &what) const
{
    int cntr = 1;

    for (const auto &v : what)
    {
        outstr << std::setprecision(4) << v.x() << ", " << std::setprecision(4) << v.y() << ", ";
        if (++cntr % 6 == 0)
        {
            outstr << std::endl;
        }
    }
}

LuaDumper::LuaDumper(std::ostream &out, const SvgProcessor &pr, const std::string &namePrefix,
                     const bool use_local) :
    IDumper(out, pr, namePrefix),
    use_local(use_local)
{
    dumpPath(pr.getTesselated());
}
