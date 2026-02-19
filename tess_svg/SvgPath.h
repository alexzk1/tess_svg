//
// Created by alex on 7/15/15.
//

#ifndef TESSVG_SVGPATH_H
#define TESSVG_SVGPATH_H

#include "GlDefs.h"
#include "pugixml.hpp"

#include <regex>

class SvgPath
{
  private:
    Loops loops;
    const pugi::xml_node parentNode;

    class func_holder
    {

      private:
        typedef std::function<void(GlVertex::trans_matrix_t &, std::sregex_token_iterator &)>
          lambda_t;
        const std::string mask;
        const lambda_t func;

      public:
        func_holder(const std::string &mask, const lambda_t &func) :
            mask(mask),
            func(func)
        {
        }
        bool exec(const std::string &src, GlVertex::trans_matrix_t &res) const;
    };
    typedef std::vector<func_holder> transform_funcs;
    const static transform_funcs transforms;

  public:
    SvgPath() = default;
    SvgPath(const SvgPath &) = default;

    SvgPath(const pugi::xml_node &node, const pugi::xml_node &parentNode);
    void parse_node(const pugi::xml_node &node);
    const Loops &getLoops() const;
};

#endif // TESSVG_SVGPATH_H
