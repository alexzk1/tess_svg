//
// Created by alex on 7/15/15.
//

#ifndef TESSVG_SVGPROCESSOR_H
#define TESSVG_SVGPROCESSOR_H

#include "processing_data.hpp"
#include "pugixml.hpp"

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

/// @brief Simple transformations (processing, like boolean intercections) chain builder.
class SvgWorldTransformers
{
  public:
    // TODO:
    SvgWorldTransformers &addTransformer();

    ///@brief Builds final surrounding polygon which is the result of this tool.
    [[nodiscard]]
    SvgWorld buildSurroundingPolygons(SvgWorld world);

  private:
};

/// @brief Load SVG file/data for further processing, it does parsing of it like accounting nested
/// translations.
/// @return SvgWolrd object which contains loaded SVG data translated into World coordinates.
SvgWorld loadSvgWorld(std::istream &src);

#endif // TESSVG_SVGPROCESSOR_H
