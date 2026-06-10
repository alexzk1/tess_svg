//
// Created by alex on 7/15/15.
//

#ifndef TESSVG_SVGPATH_H
#define TESSVG_SVGPATH_H

#include "GlDefs.h"
#include "pugixml.hpp"

#include <functional>
#include <regex>
#include <string>
#include <utility>
#include <vector>

class SvgPath
{
  protected:
    class func_holder
    {
      public:
        using lambda_t =
          std::function<void(GlVertex::trans_matrix_t &, std::sregex_token_iterator &)>;

        func_holder(std::string mask, lambda_t func) :
            mask(std::move(mask)),
            func(std::move(func))
        {
        }
        bool exec(const std::string &src, GlVertex::trans_matrix_t &res) const;

      private:
        const std::string mask;
        const lambda_t func;
    };
    using transform_funcs = std::vector<func_holder>;
    const static transform_funcs transforms;

  public:
    SvgPath() = default;
    SvgPath(const SvgPath &) = default;

    SvgPath(const pugi::xml_node &node, const pugi::xml_node &parentNode);
    void parse_node(const pugi::xml_node &node);
    [[nodiscard]]
    const Loops &getLoops() const;

  private:
    void apply_transform_attr(const std::string &attr_value, GlVertex::trans_matrix_t &vtr);

    Loops loops;
    const pugi::xml_node parentNode;
};

#endif // TESSVG_SVGPATH_H
