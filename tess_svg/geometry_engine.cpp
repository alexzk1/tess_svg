#include "geometry_engine.hpp"

#include "tess_svg/GlDefs.h"

#include <clipper2/clipper.core.h>
#include <clipper2/clipper.h>

#include <algorithm>
#include <utility>
#include <vector>

namespace geometry_engine {

namespace {

// FIXME: is it ok ?
constexpr int kPrecision =
  std::min(Clipper2Lib::CLIPPER2_MAX_DEC_PRECISION, Vertexes::value_type::kPrecision);

Clipper2Lib::PathsD toPathsD(const Loops &loops)
{
    using ValueT = decltype(Clipper2Lib::PathD::value_type::x);

    Clipper2Lib::PathsD paths;
    for (const auto &vertexes : loops)
    {
        Clipper2Lib::PathD path;
        path.reserve(vertexes.size());
        for (const auto &v : vertexes)
        {
            path.emplace_back(static_cast<ValueT>(v.x()), static_cast<ValueT>(v.y()));
        }
        paths.push_back(std::move(path));
    }
    return paths;
}

Loops fromPathsD(const Clipper2Lib::PathsD &paths)
{
    using ValueT = Vertexes::value_type::value_t;

    Loops loops;
    loops.reserve(paths.size());
    for (const auto &path : paths)
    {
        Vertexes v_list;
        v_list.reserve(path.size());
        for (const auto &pt : path)
        {
            v_list.emplace_back(static_cast<ValueT>(pt.x), static_cast<ValueT>(pt.y));
        }
        loops.push_back(std::move(v_list));
    }
    return loops;
}
} // namespace

Loops unionShapes(const std::vector<Loops> &shapes)
{
    if (shapes.empty())
    {
        return {};
    }

    Clipper2Lib::PathsD all_subjects;
    for (const auto &l : shapes)
    {
        auto p = toPathsD(l);
        all_subjects.insert(all_subjects.end(), p.begin(), p.end());
    }

    const auto result =
      Clipper2Lib::Union(all_subjects, Clipper2Lib::FillRule::NonZero, kPrecision);
    return fromPathsD(result);
}

Loops intersectWithMasks(const Loops &subject, const std::vector<Loops> &masks)
{
    if (masks.empty())
    {
        return subject;
    }
    const Clipper2Lib::PathsD subjectPath = toPathsD({subject});
    Clipper2Lib::PathsD maskPaths;
    for (const auto &m : masks)
    {
        auto p = toPathsD(m);
        maskPaths.insert(maskPaths.end(), p.begin(), p.end());
    }

    const auto result =
      Clipper2Lib::Intersect(subjectPath, maskPaths, Clipper2Lib::FillRule::NonZero, kPrecision);
    return fromPathsD(result);
}

} // namespace geometry_engine
