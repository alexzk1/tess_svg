//
// Created by alex on 7/15/15.
//

#ifndef TESSVG_SVGPATH_H
#define TESSVG_SVGPATH_H

#include "GlDefs.h"

#include <pugixml.hpp>

class SvgPath
{
  public:
    SvgPath() = default;
    SvgPath(const SvgPath &) = default;

    SvgPath(const pugi::xml_node &node, const pugi::xml_node &parentNode);
    void parse_node(const pugi::xml_node &node);
    [[nodiscard]]
    const Loops &getLoops() const;

  private:
    Loops loops;
    const pugi::xml_node parentNode;
};

#endif // TESSVG_SVGPATH_H
