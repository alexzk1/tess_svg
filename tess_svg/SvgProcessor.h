//
// Created by alex on 7/15/15.
//

#ifndef TESSVG_SVGPROCESSOR_H
#define TESSVG_SVGPROCESSOR_H

#include "SvgParsers.h"
#include "processing_data.hpp"
#include "pugixml.hpp"

#include <cstddef>
#include <istream>
#include <stdexcept>
#include <string>

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
  private:
    struct RecursionParameters;
    void parse(std::size_t recursionLevel, const pugi::xml_node &node, RecursionParameters &params);

    pugi::xml_document doc;
    SvgWorld tesselated;

  public:
    SvgProcessor() = default;
    explicit SvgProcessor(std::istream &src);
    void parse_svg_file(std::istream &src);
    [[nodiscard]]
    const SvgWorld &getTesselated() const;
};

#endif // TESSVG_SVGPROCESSOR_H
