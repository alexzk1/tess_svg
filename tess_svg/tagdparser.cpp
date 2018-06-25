#include <iostream>
#include <vector>
#include <regex>

#include "tagdparser.h"

int BEIZER_PARTS   = 10;
int ELLIPSE_POINTS = 1024;

static bool checkIfDouble(const std::string& val) noexcept
{
    try
    {
        std::stod(val);
        return true;
    }
    catch (...)
    {
    }
    return false;
}


static bool checkIfRelative(const std::string& val)
{
    if (val.size() == 1)
        return val[0] >= 'a' && val[0] <= 'z';
    throw std::runtime_error("checkIfRelative called for wrong literals: " + val);
}

Loops TagDParser::split(const std::string &src, const GlVertex::trans_matrix_t &translate)
{
    std::regex add_space("([a-zA-Z])([0-9|\\.|\\-])");
    std::string srccopy = std::regex_replace(src, add_space, "$1 $2");

    //std::cout<<"Spaced:\n"<<srccopy<<std::endl;

    std::regex rgx("\\s+|\\,+");
    std::sregex_token_iterator iter(srccopy.cbegin(),
                                    srccopy.cend(),
                                    rgx, -1);
    std::sregex_token_iterator end;

    std::vector<std::string> strs;
    for ( ; iter != end; ++iter)
    {
        //std::cout<<*iter << '\n';
        strs.push_back(*iter);
    }

    Loops result;

    GlVertex lastVert(0, 0);
    GlVertex lastInitial = lastVert;
    std::shared_ptr<GlVertex> lastMirrorQuadratic = nullptr;
    std::shared_ptr<GlVertex> lastMirrorCubic     = nullptr;

    Vertexes current_path;
    std::string curr;

    bool path_started = true;

    for (size_t i = 0, n = strs.size(); i < n; i++)
    {

        if (checkIfDouble(strs.at(i)))
            i--;
        else
            curr =  strs.at(i);

        const bool is_rel = checkIfRelative(curr);
        GlVertex delta = (is_rel) ? lastVert : GlVertex(0, 0);


        if (curr == "z" || curr == "Z")
        {
            if (current_path.size() < 1)
                throw std::runtime_error("Closing Z detected, but have less than 2 points yet.");

            result.push_back(current_path);
            current_path.clear();
            //If a "closepath" is followed immediately by any other command, then the next subpath starts at the same initial point as the current subpath
            lastVert = lastInitial;
            path_started = true;
            continue;
        }
        else
        {
            if (curr == "Q" || curr == "q")
            {
                auto x1y1 = delta.get() + getXY(strs, i).get();
                lastMirrorQuadratic.reset(new GlVertex(x1y1));

                auto xy  = delta.get() + getXY(strs, i).get();
                quadraticBeizer(lastVert, x1y1, xy, current_path);
                lastVert = xy;
                continue;
            }

            if (curr == "T" || curr == "t")
            {
                auto xy  = delta.get() + getXY(strs, i).get();
                auto x1y1 = delta.get() + lastVert.get();

                if (lastMirrorQuadratic != nullptr)
                    x1y1 = delta.get() + lastMirrorQuadratic->mirror(lastVert).get();


                lastMirrorQuadratic.reset(new GlVertex(x1y1));

                quadraticBeizer(lastVert, x1y1, xy, current_path);
                lastVert = xy;
                continue;
            }

            if (curr == "C" || curr == "c")
            {
                auto x1y1 = delta.get() + getXY(strs, i).get();
                auto x2y2 = delta.get() + getXY(strs, i).get();
                lastMirrorCubic.reset(new GlVertex(x1y1));

                auto xy  = delta.get() + getXY(strs, i).get();
                cubicBeizer(lastVert, x1y1, x2y2, xy, current_path);
                lastVert = xy;
                continue;
            }

            if (curr == "S" || curr == "s")
            {
                auto x2y2 = delta.get() + getXY(strs, i).get();
                auto xy  = delta.get() + getXY(strs, i).get();
                auto x1y1 = delta.get() + lastVert.get();

                if (lastMirrorCubic != nullptr)
                    x1y1 = delta.get() + lastMirrorCubic->mirror(lastVert).get();

                lastMirrorCubic.reset(new GlVertex(x1y1));
                cubicBeizer(lastVert, x1y1, x2y2, xy, current_path);

                lastVert = xy;
                continue;
            }

            if (curr == "A" || curr == "a")
            {
                auto rxy    = getXY(strs, i).get();
                auto xrot   = getX(strs, i).x();
                auto laFlag = getX(strs, i).x();
                auto sweepFlag = getX(strs, i).x();
                auto xy = getXY(strs, i).get();
                std::cerr << "Warning! Rasterizing of ellipse is not implemented yet." << std::endl;
                continue;
            }

            if (curr == "M" || curr == "L" || curr == "m" || curr == "l")
            {
                if (path_started)
                    delta = GlVertex(0, 0);

                lastVert = GlVertex(delta.get() + getXY(strs, i).get());
                if (curr == "M" || curr == "m")
                {
                    current_path.clear();
                    lastInitial = lastVert;
                }
                curr = (path_started || checkIfRelative(curr)) ? "l" : "L";
                path_started = false;
            }
            else
            {
                if ( curr == "H" || curr == "h" || curr == "V" || curr == "v")
                {
                    GlVertex tmp;
                    if (curr == "H" || curr == "h")
                    {
                        tmp = getX(strs, i);
                        tmp = GlVertex(tmp.x(), (is_rel) ? 0 : lastVert.y());
                    }
                    else
                    {
                        tmp = getY(strs, i);
                        tmp = GlVertex((is_rel) ? 0 : lastVert.x(), tmp.y());
                    }
                    lastVert = GlVertex(delta.get() + tmp.get());
                }
                else
                {
                    std::cerr << "Source: " << src << std::endl;
                    throw std::runtime_error("Uknown SVG command: " + curr);
                }
            }
        }

        current_path.push_back(lastVert);
        lastMirrorQuadratic.reset();
        lastMirrorCubic.reset();
    }

    //bad formed path ? dont have Z at the end

    if (current_path.size())
        result.push_back(current_path);

    //    std::cerr << "Has "<<result.back().size()<< " points\n";
    //    for (auto& p : result.back())
    //    {
    //        std::cerr << p.x() <<"; " << p.y() << std::endl;
    //    }

    for (auto& r : result)
    {
        for (auto& v : r)
            v.translate(translate);
    }

    return result;
}


GlVertex TagDParser::getXY(std::vector<std::string> &src, size_t &index)
{
    index += 2;
    return GlVertex(src.at(index - 1), src.at(index));
}

GlVertex TagDParser::getX(std::vector<std::string> &src, size_t &index)
{
    return GlVertex(src.at(++index), "0");
}

GlVertex TagDParser::getY(std::vector<std::string> &src, size_t &index)
{
    return GlVertex("0", src.at(++index));
}

void TagDParser::quadraticBeizer(const GlVertex &x0y0, const GlVertex &x1y1, const GlVertex &xy, Vertexes& path)
{
    for (auto step = 0; step <= BEIZER_PARTS; step++)
    {
        auto t = (GLdouble)step / (GLdouble)BEIZER_PARTS;
        auto t1 = 1 - t;
        GlVertex point = t1 * t1 * x0y0.get() + 2 * t * t1 * x1y1.get() + t * t * xy.get();

        path.push_back(point);
    }
}

void TagDParser::cubicBeizer(const GlVertex &x0y0, const GlVertex &x1y1, const GlVertex &x2y2, const GlVertex &xy, Vertexes &path)
{
    for (auto step = 0; step <= BEIZER_PARTS; step++)
    {
        auto t = (GLdouble)step / (GLdouble)BEIZER_PARTS;
        auto t1 = 1 - t;
        GlVertex point = t1 * t1 * t1 * x0y0.get() + 3 * t * t1 * t1 * x1y1.get() + 3 * t * t * t1 * x2y2.get() + t * t * t * xy.get();
        path.push_back(point);
    }
}

