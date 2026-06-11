//
// Created by alex on 7/15/15.
//

#ifndef TESSVG_SVGPROCESSOR_H
#define TESSVG_SVGPROCESSOR_H

#include "GlDefs.h"
#include "SvgParsers.h"
#include "Tesselate.h"
#include "pugixml.hpp"

#include <cstddef>
#include <istream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

class xmlerror : public std::runtime_error
{
  public:
    explicit xmlerror(const std::string &_arg) :
        runtime_error(_arg)
    {
    }
};

class SvgProcessor
{
  public:
    struct TessResult
    {
        void setAttributes(const pugi::xml_node &node)
        {
            attributes.clear();

            for (auto attr = node.attributes_begin(); attr != node.attributes_end(); attr++)
            {
                attributes[std::string(attr->name())] = std::string(attr->as_string());
            }
        }
        std::map<std::string, std::string> attributes;
        Vertexes vertexes;
    };

    struct BoundedGroup
    {
        using pathes_t = std::vector<std::pair<std::string, TessResult>>;
        pathes_t pathes;
    };

    using group_t = std::vector<std::pair<std::string, BoundedGroup>>;

  private:
    struct RecursionParameters;
    void parse(std::size_t recursionLevel, const pugi::xml_node &node, RecursionParameters &params);

    Tesselate ts;
    pugi::xml_document doc;
    group_t tesselated;

  public:
    SvgProcessor() = default;
    explicit SvgProcessor(std::istream &src);
    void parse_svg_file(std::istream &src);
    [[nodiscard]]
    const group_t &getTesselated() const;
};

#endif // TESSVG_SVGPROCESSOR_H
