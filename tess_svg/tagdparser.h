#ifndef TAGDPARSER_H
#define TAGDPARSER_H
#include <string>
#include <vector>
#include <cstdint>

#include "GlDefs.h"
//http://www.w3.org/TR/SVG11/paths.html

class TagDParser
{
private:
    static GlVertex getXY(std::vector<std::string>& src, size_t& index);
    static GlVertex getX(std::vector<std::string>& src, size_t& index);
    static GlVertex getY(std::vector<std::string>& src, size_t& index);

    static void quadraticBeizer(const GlVertex &x0y0, const GlVertex& x1y1, const GlVertex& xy, Vertexes &path);
    static void cubicBeizer(const GlVertex &x0y0, const GlVertex& x1y1, const GlVertex& x2y2, const GlVertex& xy, Vertexes &path);
public:
    TagDParser() = default;
    static Loops split(const std::string& src, const GlVertex::trans_matrix_t& translate);
};

#endif // TAGDPARSER_H
