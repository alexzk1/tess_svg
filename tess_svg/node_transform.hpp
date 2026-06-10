#pragma once

#include "GlDefs.h"
#include "engine/my_math.h"

#include <math.h> //NOLINT
#include <pugixml.hpp>

#include <array>
#include <cstddef>
#include <functional>
#include <iterator>
#include <regex>
#include <string>
#include <utility>

namespace trans_detals {

/// @brief Provides access to transformation parsers.
inline const auto &getTransformParsers()
{
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
        bool exec(const std::string &src, GlVertex::trans_matrix_t &res) const
        {
            bool found = false;
            std::size_t start_of_args = 0;
            for (std::size_t pos = src.find(mask, start_of_args); pos != std::string::npos;
                 pos = src.find(mask, start_of_args))
            {
                found = true;
                static const std::regex rgx(R"([\s,\(\)]+)");
                // Смещаемся к началу чисел после маски
                start_of_args = pos + mask.length();
                std::sregex_token_iterator iter(std::next(src.begin(), start_of_args), src.end(),
                                                rgx, -1);

                // Пропускаем возможный пустой токен в начале (если сразу за маской разделитель)
                if (iter != std::sregex_token_iterator() && iter->str().empty())
                {
                    ++iter;
                }

                func(res, iter);
            }
            return found;
        }

      private:
        const std::string mask;
        const lambda_t func;
    };
    static const std::array<func_holder, 4u> transforms = {
      func_holder("translate(",
                  [](auto &m, auto &b) {
                      const std::sregex_token_iterator end;
                      if (b == end)
                      {
                          return;
                      }
                      const auto x = std::stod(*b++);
                      const auto y = ((end == b) ? 0.0 : std::stod(*b));

                      GlVertex::trans_matrix_t temp = GlVertex::getIdentity();
                      temp(0, 2) = x;
                      temp(1, 2) = y;
                      m = prod(m, temp); // Умножаем текущую на новую
                  }),

      func_holder("matrix(",
                  [](auto &m, auto &v2) {
                      const std::array<used_vectors_type, 6> vals = {
                        std::stod(*v2++), std::stod(*v2++), std::stod(*v2++),
                        std::stod(*v2++), std::stod(*v2++), std::stod(*v2++)};

                      GlVertex::trans_matrix_t temp = GlVertex::getIdentity();
                      for (auto j = 0; j < 3; j++)
                      {
                          temp(0, j) = vals[j * 2 + 0]; // NOLINT
                          temp(1, j) = vals[j * 2 + 1]; // NOLINT
                      }
                      m = prod(m, temp);
                  }),

      func_holder("scale(",
                  [](auto &m, auto &b) {
                      const std::sregex_token_iterator end;
                      if (b == end)
                      {
                          return;
                      }
                      const auto x = std::stod(*b++);
                      // В SVG если y не указан, он равен x
                      const auto y = (b == end) ? x : std::stod(*b);

                      GlVertex::trans_matrix_t temp = GlVertex::getIdentity();
                      temp(0, 0) = x;
                      temp(1, 1) = y;
                      m = prod(m, temp);
                  }),
      func_holder("rotate(",
                  [](auto &m, auto &b) {
                      const std::sregex_token_iterator end;
                      if (b == end)
                      {
                          return;
                      }

                      // В SVG углы в градусах, а sin/cos в радианах
                      const double angle = std::stod(*b++) * (M_PI / 180.0); // NOLINT

                      GlVertex::trans_matrix_t temp = GlVertex::getIdentity();
                      const auto sc = mymath::sincos(angle);
                      temp(0, 0) = sc.cos;
                      temp(0, 1) = -sc.sin;
                      temp(1, 0) = sc.sin;
                      temp(1, 1) = sc.cos;

                      m = prod(m, temp);
                  }),
    };
    return transforms;
}
} // namespace trans_detals

/// @brief Updates transformation @p vtr by values stored in @p node.
inline void updateTransform(const pugi::xml_node &node, GlVertex::trans_matrix_t &vtr)
{
    const auto &transforms = trans_detals::getTransformParsers();
    const std::string attr_value = node.attribute("transform").as_string("");
    if (attr_value.empty())
    {
        return;
    }

    for (auto &f : transforms)
    {
        f.exec(attr_value, vtr);
    }
}
