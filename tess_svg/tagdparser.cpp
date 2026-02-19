#include "tagdparser.h"

#include "engine/Vector2.h"
#include "engine/sincos_cached.h"
#include "tess_svg/GlDefs.h"

#include <GL/gl.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <optional>
#include <regex>
#include <stdexcept>
#include <string>
#include <vector>

// Those are set from int main()
// TODO: fix that...why do we have globals ? :D
std::size_t BEIZER_PARTS = 10;
std::size_t ELLIPSE_POINTS = 1024;

namespace {

// Rotate a point by an angle around the origin point.
template <class T>
std::array<T, 2> rotate(T x, T y, T angle)
{
    static sincos_cached<T, 10> sincos;
    return {{x * sincos.cos(angle) - y * sincos.sin(angle),
             y * sincos.cos(angle) + x * sincos.sin(angle)}};
}

// Return angle between x axis and point knowing given center.
template <class T>
T pointAngle(T cx, T cy, T px, T py)
{
    return std::atan2(py - cy, px - cx);
}

bool checkIfDouble(const std::string &val) noexcept
{
    try
    {
        std::stod(val);
        return true;
    }
    catch (...) // NOLINT
    {
    }
    return false;
}

bool checkIfRelative(const std::string &val)
{
    if (val.size() == 1)
    {
        return val[0] >= 'a' && val[0] <= 'z';
    }
    throw std::runtime_error("checkIfRelative called for wrong literals: " + val);
}
} // namespace

Loops TagDParser::split(const std::string &src, const GlVertex::trans_matrix_t &translate)
{
    static const std::regex add_space("([a-zA-Z])([0-9|\\.|\\-])");
    const std::string srccopy = std::regex_replace(src, add_space, "$1 $2");

    // std::cout<<"Spaced:\n"<<srccopy<<std::endl;

    static const std::regex rgx("\\s+|\\,+");
    std::sregex_token_iterator iter(srccopy.cbegin(), srccopy.cend(), rgx, -1);
    const std::sregex_token_iterator end;

    std::vector<std::string> strs;
    for (; iter != end; ++iter)
    {
        // std::cout<<*iter << '\n';
        strs.push_back(*iter);
    }

    Loops result;

    GlVertex lastVert(0, 0);
    GlVertex lastInitial = lastVert;
    std::optional<GlVertex> lastMirrorQuadratic{std::nullopt};
    std::optional<GlVertex> lastMirrorCubic{std::nullopt};

    Vertexes current_path;
    std::string curr;

    bool path_started = true;

    for (std::size_t i = 0, n = strs.size(); i < n; ++i)
    {

        if (checkIfDouble(strs.at(i)))
        {
            --i;
        }
        else
        {
            curr = strs.at(i);
        }

        const bool is_rel = checkIfRelative(curr);
        GlVertex delta = (is_rel) ? lastVert : GlVertex(0, 0);

        if (curr == "z" || curr == "Z")
        {
            if (current_path.size() < 1)
            {
                throw std::runtime_error("Closing Z detected, but have less than 2 points yet.");
            }

            result.push_back(current_path);
            current_path.clear();
            // If a "closepath" is followed immediately by any other command, then the next subpath
            // starts at the same initial point as the current subpath
            lastVert = lastInitial;
            path_started = true;
            continue;
        }
        else
        {
            if (curr == "Q" || curr == "q")
            {
                auto x1y1 = delta.get() + getXY(strs, i).get();
                lastMirrorQuadratic = GlVertex(x1y1);

                auto xy = delta.get() + getXY(strs, i).get();
                quadraticBeizer(lastVert, x1y1, xy, current_path);
                lastVert = xy;
                continue;
            }

            if (curr == "T" || curr == "t")
            {
                auto xy = delta.get() + getXY(strs, i).get();
                auto x1y1 = delta.get() + lastVert.get();

                if (lastMirrorQuadratic)
                {
                    x1y1 = delta.get() + lastMirrorQuadratic->mirror(lastVert).get();
                }

                lastMirrorQuadratic = GlVertex(x1y1);

                quadraticBeizer(lastVert, x1y1, xy, current_path);
                lastVert = xy;
                continue;
            }

            if (curr == "C" || curr == "c")
            {
                auto x1y1 = delta.get() + getXY(strs, i).get();
                auto x2y2 = delta.get() + getXY(strs, i).get();
                lastMirrorCubic = GlVertex(x1y1);

                auto xy = delta.get() + getXY(strs, i).get();
                cubicBeizer(lastVert, x1y1, x2y2, xy, current_path);
                lastVert = xy;
                continue;
            }

            if (curr == "S" || curr == "s")
            {
                auto x2y2 = delta.get() + getXY(strs, i).get();
                auto xy = delta.get() + getXY(strs, i).get();
                auto x1y1 = delta.get() + lastVert.get();

                if (lastMirrorCubic)
                {
                    x1y1 = delta.get() + lastMirrorCubic->mirror(lastVert).get();
                }

                lastMirrorCubic = GlVertex(x1y1);
                cubicBeizer(lastVert, x1y1, x2y2, xy, current_path);

                lastVert = xy;
                continue;
            }

            if (curr == "A" || curr == "a")
            {
                // http://xahlee.info/js/svg_path_ellipse_arc.html
                // https://dai.fmph.uniba.sk/upload/0/01/Ellipse.pdf
                // https://github.com/igagis/svgren/blob/master/src/svgren/Renderer.cpp

                auto xy = delta.get() + getXY(strs, i).get(); // now it is absolute

                // auto rxy = getXY(strs, i).get();
                // auto xrot = getX(strs, i).x();
                // bool laFlag = getX(strs, i).x();
                // bool sweepFlag = getX(strs, i).x();
                //                double radiiRatio = rxy.y / rxy.x;
                //                if (radiiRatio <= 0)
                //                    continue;
                //                //cancel rotation of end point
                //                double xe, ye;
                //                {
                //                    auto xxyy = xy.get() - lastVert.get();

                //                    auto res = rotate(xxyy.x(), xxyy.y(), deg2rad(xrot));
                //                    xe = res[0];
                //                    ye = res[1];
                //                }
                //                ye /= radiiRatio;

                //                //Find the angle between the end point and the x axis
                //                auto angle = pointAngle(double(0), double(0), xe, ye);

                //                //Put the end point onto the x axis
                //                xe = std::sqrt(xe * xe + ye * ye);
                //                ye = 0;

                //                //Update the x radius if it is too small
                //                auto rx = std::fmax(rxy.x, xe / 2.);

                //                //Find one circle center
                //                auto xc = xe / 2.;
                //                auto yc = std::sqrt(rx * rx - xc * xc);

                //                //Choose between the two circles according to flags
                //                if (!(laFlag != sweepFlag))
                //                    yc = -yc;

                //                //Put the second point and the center back to their positions
                //                {
                //                    auto res = rotate(xe, double(0), angle);
                //                    xe = res[0];
                //                    ye = res[1];
                //                }
                //                {
                //                    auto res = rotate(xc, yc, angle);
                //                    xc = res[0];
                //                    yc = res[1];
                //                }

                //                auto angle1 = pointAngle(xc, yc, double(0), double(0));
                //                auto angle2 = pointAngle(xc, yc, xe, ye);

                std::cerr << "Arc tag A/a is not implemented yet!" << std::endl;
                lastVert = xy;
                continue;
            }

            if (curr == "M" || curr == "L" || curr == "m" || curr == "l")
            {
                if (path_started)
                {
                    delta = GlVertex(0, 0);
                }

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
                if (curr == "H" || curr == "h" || curr == "V" || curr == "v")
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

    // bad formed path ? dont have Z at the end

    if (current_path.size())
    {
        result.push_back(current_path);
    }

    //    std::cerr << "Has " << result.back().size() << " points\n";
    //    for (auto& p : result.back())
    //        std::cerr << p.x() << "; " << p.y() << std::endl;

    for (auto &r : result)
    {
        for (auto &v : r)
        {
            v.translate(translate);
        }
    }

    return result;
}

GlVertex TagDParser::getXY(std::vector<std::string> &src, size_t &index)
{
    index += 2;
    return {src.at(index - 1), src.at(index)};
}

GlVertex TagDParser::getX(std::vector<std::string> &src, size_t &index)
{
    return {src.at(++index), "0"};
}

GlVertex TagDParser::getY(std::vector<std::string> &src, size_t &index)
{
    return {"0", src.at(++index)};
}

void TagDParser::quadraticBeizer(const GlVertex &x0y0, const GlVertex &x1y1, const GlVertex &xy,
                                 Vertexes &path)
{
    for (std::size_t step = 0; step <= BEIZER_PARTS; ++step)
    {
        const auto t = static_cast<GLdouble>(step) / static_cast<GLdouble>(BEIZER_PARTS);
        const auto t1 = 1 - t;
        const GlVertex point = t1 * t1 * x0y0.get() + 2 * t * t1 * x1y1.get() + t * t * xy.get();
        path.emplace_back(point);
    }
}

void TagDParser::cubicBeizer(const GlVertex &x0y0, const GlVertex &x1y1, const GlVertex &x2y2,
                             const GlVertex &xy, Vertexes &path)
{
    for (std::size_t step = 0; step <= BEIZER_PARTS; ++step)
    {
        const auto t = static_cast<GLdouble>(step) / static_cast<GLdouble>(BEIZER_PARTS);
        const auto t1 = 1 - t;
        const GlVertex point = t1 * t1 * t1 * x0y0.get() + 3 * t * t1 * t1 * x1y1.get()
                               + 3 * t * t * t1 * x2y2.get() + t * t * t * xy.get();
        path.emplace_back(point);
    }
}
