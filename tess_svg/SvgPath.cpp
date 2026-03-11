//
// Created by alex on 7/15/15.
//

#include "SvgPath.h"

#include "tagdparser.h"
#include "tess_svg/GlDefs.h"
#include "util_helpers.h"

#include <array>
#include <cstddef>
#include <iterator>
#include <regex>
#include <type_traits>

namespace {

/// @brief Result structure for simultaneous sine and cosine calculation.
template <class taFloatingType>
struct sincos_res
{
    static_assert(std::is_floating_point_v<taFloatingType>,
                  "taFloatingType must be a floating point type.");
    taFloatingType sin;
    taFloatingType cos;
};

/// @brief Computes sine and cosine simultaneously.
/// Uses platform-specific optimizations (GNU sincosf) where available.
/// @param angle_rad Angle in radians.
template <class taFloatingType>
inline sincos_res<taFloatingType> sincos(taFloatingType angleRadians)
{
    static_assert(std::is_floating_point_v<taFloatingType>,
                  "taFloatingType must be a floating point type.");
    sincos_res<taFloatingType> res; // NOLINT

    // Для Android NDK и Linux (GCC/Clang)
#if defined(__GNUC__) || defined(__clang__)
    if constexpr (std::is_same_v<taFloatingType, float>)
    {
        ::sincosf(angleRadians, &res.sin, &res.cos); // NOLINT
    }
    else if constexpr (std::is_same_v<taFloatingType, double>) // NOLINT
    {
        ::sincos(angleRadians, &res.sin, &res.cos); // NOLINT
    }
    else
    {
        ::sincosl(angleRadians, &res.sin, &res.cos); // NOLINT
    }
#else
    // Fallback for MSVC or other compilers without GNU.
    #ifndef SFW_SINCOS_WARNED
        #define SFW_SINCOS_WARNED
        #pragma message(                                                                           \
          "Warning: math::sincos is using slow std::sin/std::cos fallback on this compiler.")
    #endif
    res.sin = std::sin(angleRadians);
    res.cos = std::cos(angleRadians);
#endif
    return res;
}
} // namespace

const SvgPath::transform_funcs SvgPath::transforms = {
  SvgPath::func_holder("translate(",
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

  SvgPath::func_holder("matrix(",
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

  SvgPath::func_holder("scale(",
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
  SvgPath::func_holder("rotate(",
                       [](auto &m, auto &b) {
                           const std::sregex_token_iterator end;
                           if (b == end)
                           {
                               return;
                           }

                           // В SVG углы в градусах, а sin/cos в радианах
                           const double angle = std::stod(*b++) * (M_PI / 180.0); // NOLINT

                           GlVertex::trans_matrix_t temp = GlVertex::getIdentity();
                           const auto sc = sincos(angle);
                           temp(0, 0) = sc.cos;
                           temp(0, 1) = -sc.sin;
                           temp(1, 0) = sc.sin;
                           temp(1, 1) = sc.cos;

                           m = prod(m, temp);
                       }),
};

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
SvgPath::SvgPath(const pugi::xml_node &node, const pugi::xml_node &parentNode) :
    parentNode(parentNode)
{
    parse_node(node);
}

void SvgPath::parse_node(const pugi::xml_node &node)
{
    GlVertex::trans_matrix_t vtr = GlVertex::getIdentity();

    loops.clear();

    // 1. Сначала применяем трансформ родителя
    apply_transform_attr(parentNode.attribute("transform").as_string(""), vtr);

    // 2. Затем ПОВЕРХ применяем трансформ самой ноды (умножение произойдет внутри)
    apply_transform_attr(node.attribute("transform").as_string(""), vtr);

    if (toLower(node.name()) == "path")
    {
        loops = TagDParser::split(node.attribute("d").as_string(), vtr);
    }
}

const Loops &SvgPath::getLoops() const
{
    return loops;
}

void SvgPath::apply_transform_attr(const std::string &attr_value, GlVertex::trans_matrix_t &vtr)
{
    if (attr_value.empty())
    {
        return;
    }

    for (auto &f : transforms)
    {
        f.exec(attr_value, vtr);
    }
}

bool SvgPath::func_holder::exec(const std::string &src, GlVertex::trans_matrix_t &res) const
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
        std::sregex_token_iterator iter(std::next(src.begin(), start_of_args), src.end(), rgx, -1);

        // Пропускаем возможный пустой токен в начале (если сразу за маской разделитель)
        if (iter != std::sregex_token_iterator() && iter->str().empty())
        {
            ++iter;
        }

        func(res, iter);
    }
    return found;
}
