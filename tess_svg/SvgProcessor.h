//
// Created by alex on 7/15/15.
//

#ifndef TESSVG_SVGPROCESSOR_H
#define TESSVG_SVGPROCESSOR_H

#include "processing_data.hpp"
#include "pugixml.hpp"

#include <functional>
#include <istream>
#include <stdexcept>
#include <string>
#include <vector>

class xmlerror : public std::runtime_error
{
  public:
    explicit xmlerror(const std::string &_arg) :
        runtime_error(_arg)
    {
    }
};

/// @brief Single transactional transformation of the World.
using SvgWorldTransformer = std::function<void(SvgWorld &)>;

/// @brief Simple transformations (processing, like boolean intercections) chain builder.
class SvgWorldTransformers
{
  public:
    SvgWorldTransformers &addTransformer(SvgWorldTransformer trans);

    ///@brief Builds final surrounding polygon which is the result of this tool.
    [[nodiscard]]
    SvgWorld buildSurroundingPolygons(SvgWorld world);

  private:
    std::vector<SvgWorldTransformer> transformers;
};

/// @brief Load SVG file/data for further processing, it does parsing of it like accounting nested
/// translations.
/// @return SvgWolrd object which contains loaded SVG data translated into World coordinates.
SvgWorld loadSvgWorld(std::istream &src);

/**
 * @brief Merges all primitives within each group into a single set of contours (Loops).
 * This transforms <g><rect/><circle/></g> into one object with 2 Loops (or one merged loop).
 */
void unionElementsTransformer(SvgWorld &world);

/**
 * @brief Applies masks from <defs> to the scene elements.
 * If a mask is a group, it will be automatically merged into a single contour before clipping.
 */
void clipByDefsTransformer(SvgWorld &world);

#endif // TESSVG_SVGPROCESSOR_H
