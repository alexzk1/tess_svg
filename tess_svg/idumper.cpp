#include <utility>

#include "idumper.h"
#include <iomanip>
#include "json.hpp"

bool USE_PATH_COMMENT = true;

IDumper::IDumper(std::ostream &out, const SvgProcessor &, std::string namePrefix):
    outstr(out),
    namePrefix(std::move(namePrefix))
{
}

static std::string fixClassName(std::string cname)
{
    cname.erase(remove_if(cname.begin(), cname.end(), [](auto c)->bool
    {
        bool r = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
        return !r;
    }), cname.end());

    if (!cname.empty())
    {
        if (!std::isalpha(cname[0]))
            cname = "C" + cname;
        cname[0] = std::toupper(cname[0]);
    }
    return cname;
}
//************************************************************************************************************************************************************
void JavaDumper::dumpPath(const SvgProcessor::group_t &what) const
{
    using namespace std;

    //java has rules about class names used...so lets fix it.
    auto cname = fixClassName(namePrefix);

    outstr << "class " << ((cname.empty()) ? "Default" : cname) << " {" << endl;
    for (const auto& g : what)
    {
        bool morethan1 = g.second.pathes.size() > 1;

        if (morethan1)
        {
            auto center = g.second.bounds.get_center();
            outstr << "//group: " << g.first << " (outer shape of the image)" << std::endl;
            outstr << "public ModelPolygon group_" << g.first << " = new ModelPolygon(";
            dumpVertexes(g.second.vertexes);
            outstr << ").setOrigin(" << std::setprecision(4) << center.x() << "f," << std::setprecision(4) << center.y() << "f);" << endl << endl;
            outstr << "//end-of-group: " << g.first << std::endl;
            outstr << "//Now each path separated if you need it: " << std::endl;
            if (USE_PATH_COMMENT)
                outstr << "/*" << std::endl;
        }
        for (const auto& p : g.second.pathes)
        {
            auto &tess_result = p.second; //TessResult
            auto center = tess_result.bounds.get_center();
            outstr << "public ModelPolygon " << p.first << " = new ModelPolygon(";
            dumpVertexes(tess_result.vertexes);
            outstr << ").setOrigin(" << std::setprecision(4) << center.x() << "f," << std::setprecision(4) << center.y() << "f);" << endl << endl;
        }
        if (morethan1 && USE_PATH_COMMENT)
            outstr << "*/" << std::endl;
    }
    outstr << "};" << endl;
}

void JavaDumper::dumpVertexes(const Vertexes &what) const
{
    int cntr = 1;
    outstr << "new float[]{";

    for (const auto& v : what)
    {
        outstr << std::setprecision(4) << v.x() << "f, " << std::setprecision(4) << v.y() << "f, ";
        if (++cntr % 6 == 0)
            outstr << std::endl;
    }
    outstr << "}";
}

JavaDumper::JavaDumper(std::ostream &out, const SvgProcessor &pr, const std::string &namePrefix):
    IDumper(out, pr, namePrefix)
{
    dumpPath(pr.getTesselated());
}

//************************************************************************************************************************************************************
void JsonDumper::dumpPath(const SvgProcessor::group_t &what) const
{
    using namespace std;
    using namespace nlohmann;

    const static auto out_point  = [](const auto & point)
    {
        return json{point.x(), point.y()};
    };

    const auto out_vertex = [](const Vertexes & what)->json
    {
        json vertex = json::array();
        for (const auto & v : what)
        {
            vertex.push_back(v.x());
            vertex.push_back(v.y());
        }
        return vertex;
    };

    const auto out_polygon = [&out_vertex](const auto & g)
    {
        const auto center = g.second.bounds.get_center();
        json poly = json::object();
        poly["vertexes"] = out_vertex(g.second.vertexes);
        poly["origin"]   = out_point(center);
        return poly;
    };

    json object = json::object();

    for (const auto& g : what)
    {
        bool morethan1 = g.second.pathes.size() > 1;

        if (morethan1)
        {
            json group = json::object();
            group["outline"] = out_polygon(g);

            json cont = json::object();
            for (const auto& p : g.second.pathes)
                cont[p.first] = out_polygon(p);
            group["contains"] = cont;
            object[g.first] = group;
        }
        else
        {
            const auto& p = g.second.pathes.at(0);
            object[p.first]  = out_polygon(p);
        }

    }

    json obj2 = json::object();

    if (!namePrefix.empty())
    {
        obj2["namespace"] = true;
        obj2[namePrefix]  = object;
    }

    const auto& oj = (namePrefix.empty()) ? object : obj2;
    if (pretty)
        outstr << std::setw(4) << oj;
    else
        outstr << oj;
}

void JsonDumper::dumpVertexes(const Vertexes &) const
{
}

JsonDumper::JsonDumper(std::ostream &out, const SvgProcessor &pr, const std::string &namePrefix, bool pretty):
    IDumper(out, pr, namePrefix), pretty(pretty)
{
    dumpPath(pr.getTesselated());
}

//************************************************************************************************************************************************************
void SFMLDumper::dumpPath(const SvgProcessor::group_t &what) const
{
    using namespace std;

    //java has rules about class names used...so lets fix it.
    auto cname = fixClassName(namePrefix);

    outstr << "#include \"sf_polygon.h\"" << endl << endl << endl;
    outstr << "namespace " << ((cname.empty()) ? "sfml_default" : cname) << " {" << endl;
    for (const auto& g : what)
    {
        bool morethan1 = g.second.pathes.size() > 1;

        if (morethan1)
        {
            const auto name = "group_" + g.first;
            auto center = g.second.bounds.get_center();
            outstr << "//group: " << g.first << " (outer shape of the image)" << std::endl;
            outstr << "extern const SfPolygon " << name << "{";
            dumpVertexes(g.second.vertexes);
            outstr << "};" << endl << name << ".setOrigin(" << std::setprecision(4) << center.x() << "," << std::setprecision(4) << center.y() << ");" << endl << endl;
            outstr << "//end-of-group: " << g.first << std::endl;
            outstr << "//Now each path separated if you need it: " << std::endl;
            if (USE_PATH_COMMENT)
                outstr << "/*" << std::endl;
        }
        for (const auto& p : g.second.pathes)
        {
            auto &tess_result = p.second; //TessResult
            auto center = tess_result.bounds.get_center();
            outstr << "extern const SfPolygon " << p.first << "{";
            dumpVertexes(tess_result.vertexes);
            outstr << "};" << endl << p.first << ".setOrigin(" << std::setprecision(4) << center.x() << "," << std::setprecision(4) << center.y() << ");" << endl << endl;
        }
        if (morethan1 && USE_PATH_COMMENT)
            outstr << "*/" << std::endl;
    }
    outstr << "};" << endl;
}

void SFMLDumper::dumpVertexes(const Vertexes &what) const
{
    int cntr = 1;

    for (const auto& v : what)
    {
        outstr << std::setprecision(4) << v.x() << ", " << std::setprecision(4) << v.y() << ", ";
        if (++cntr % 6 == 0)
            outstr << std::endl;
    }
}

SFMLDumper::SFMLDumper(int, std::ostream &out, const SvgProcessor &pr, const std::string &namePrefix):
    IDumper(out, pr, namePrefix)
{

}

SFMLDumper::SFMLDumper(std::ostream &out, const SvgProcessor &pr, const std::string &namePrefix):
    IDumper(out, pr, namePrefix)
{
    dumpPath(pr.getTesselated());
}

//************************************************************************************************************************************************************
void SFMLMapDumper::dumpPath(const SvgProcessor::group_t &what) const
{
    using namespace std;

    //java has rules about class names used...so lets fix it.
    auto cname = fixClassName(namePrefix);

    outstr << "#include <map>" << endl;
    outstr << "#include <string>" << endl;
    outstr << "#include \"sf_polygon.h\"" << endl << endl << endl;
    outstr << "extern const std::map<std::string, SfPolygon> " << ((cname.empty()) ? "sfml_default" : cname) << " {" << endl;
    for (const auto& g : what)
    {
        bool morethan1 = g.second.pathes.size() > 1;

        if (morethan1)
        {
            const auto name = "group_" + g.first;
            auto center = g.second.bounds.get_center();
            outstr << "//group: " << g.first << " (outer shape of the image)" << std::endl;
            outstr << "{\"" << name << "\", {{";
            dumpVertexes(g.second.vertexes);
            outstr << "}, " << std::setprecision(4) << center.x() << "," << std::setprecision(4) << center.y() << " }}," << endl << endl;
            outstr << "//end-of-group: " << g.first << std::endl;
            outstr << "//Now each path separated if you need it: " << std::endl;
            if (USE_PATH_COMMENT)
                outstr << "/*" << std::endl;
        }


        for (const auto& p : g.second.pathes)
        {
            auto &tess_result = p.second; //TessResult
            auto center = tess_result.bounds.get_center();
            outstr << "{\"" << p.first << "\", {{";
            dumpVertexes(tess_result.vertexes);
            outstr << "}, " << std::setprecision(4) << center.x() << "," << std::setprecision(4) << center.y() << " }}," << endl << endl;
        }

        if (morethan1 && USE_PATH_COMMENT)
            outstr << "*/" << std::endl;
    }
    outstr << "};" << endl;
}

SFMLMapDumper::SFMLMapDumper(std::ostream &out, const SvgProcessor &pr, const std::string &namePrefix):
    SFMLDumper(1, out, pr, namePrefix)
{
    dumpPath(pr.getTesselated());
}


