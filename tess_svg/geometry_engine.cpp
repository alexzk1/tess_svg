#include "geometry_engine.hpp"

#include "tess_svg/GlDefs.h"
#include "tess_svg/util_helpers.h"

#include <clipper2/clipper.core.h>
#include <clipper2/clipper.engine.h>
#include <clipper2/clipper.h>

#include <algorithm>
#include <functional>
#include <iterator>
#include <utility>
#include <vector>

namespace geometry_engine {

namespace {

// FIXME: is it ok ?
constexpr int kPrecision =
  std::min(Clipper2Lib::CLIPPER2_MAX_DEC_PRECISION, Polyline::value_type::kPrecision);

Clipper2Lib::PathsD toPathsD(const Loops &loops)
{
    using ValueT = decltype(Clipper2Lib::PathD::value_type::x);

    Clipper2Lib::PathsD paths;
    for (const auto &Polyline : loops)
    {
        Clipper2Lib::PathD path;
        path.reserve(Polyline.size());
        for (const auto &v : Polyline)
        {
            path.emplace_back(static_cast<ValueT>(v.x()), static_cast<ValueT>(v.y()));
        }
        paths.push_back(std::move(path));
    }
    return paths;
}

Clipper2Lib::PathsD toPathsD(const std::vector<Loops> &shapes)
{
    if (shapes.empty())
    {
        return {};
    }
    Clipper2Lib::PathsD res;
    for (const auto &shape : shapes)
    {
        appendVectors(res, toPathsD(shape));
    }

    return res;
}

Loops fromPathsD(const Clipper2Lib::PathsD &paths)
{
    using ValueT = Polyline::value_type::value_t;

    Loops loops;
    loops.reserve(paths.size());
    for (const auto &path : paths)
    {
        Polyline v_list;
        v_list.reserve(path.size());
        for (const auto &pt : path)
        {
            v_list.emplace_back(static_cast<ValueT>(pt.x), static_cast<ValueT>(pt.y));
        }
        loops.push_back(std::move(v_list));
    }
    return loops;
}

void appendClipperPathToPolyline(const Clipper2Lib::PolyPathD &polyPath, Loops &totalLoops)
{
    using ValueT = Polyline::value_type::value_t;
    Polyline out;
    out.reserve(polyPath.Polygon().size());
    for (const auto &pt : polyPath.Polygon())
    {
        // Приведение типов из double в float для нашего Polyline
        out.emplace_back(static_cast<ValueT>(pt.x), static_cast<ValueT>(pt.y));
    }
    totalLoops.emplace_back(std::move(out));
}

// Рекурсивный обход дерева для сбора островов и их дырок
// Функция находит все "острова" (Islands) и собирает всё содержимое вокруг них.
void gatherIslands(const Clipper2Lib::PolyPathD &parentNode, std::vector<Loops> &islands)
{
    for (const auto &child : parentNode)
    { // Проход по всем детям текущего узла
        if (!child->IsHole())
        {
            // НАЙДЕН НОВЫЙ ОСТРОВ! Создаем для него новый объект.
            Loops current_island;

            // А) Добавляем внешний контур этого острова
            appendClipperPathToPolyline(*child, current_island);

            // Б) Собираем все ПРЯМЫЕ дырки (holes), принадлежащие этому острову
            for (const auto &hole : *child)
            {
                if (hole->IsHole())
                {
                    appendClipperPathToPolyline(*hole, current_island);

                    // ВАЖНО: Если внутри этой дырки есть еще один ОСТРОВ,
                    // мы должны зайти в него рекурсивно, чтобы создать новый объект.
                    gatherIslands(*hole, islands);
                }
            }

            islands.emplace_back(std::move(current_island));
        }
        else
        {
            // Если текущий узел — это дырка (но не "дочерняя" для нашего острова),
            // мы просто идем глубже, чтобы найти остров внутри этой дырки.
            gatherIslands(*child, islands);
        }
    }
}

} // namespace

std::vector<Loops> unionShapes(const std::vector<Loops> &shapes)
{
    if (shapes.empty())
    {
        return {};
    }

    // 1. Подготовка: Собираем все контуры в один массив для Clipper
    const auto all_subjects = toPathsD(shapes);
    // 2. Инициализация Clipper и выполнение Union
    Clipper2Lib::ClipperD clipper(kPrecision);
    clipper.AddSubject(all_subjects); // Используем AddSubject, чтобы сработал внутренний Scale

    Clipper2Lib::PolyTreeD polytree;
    // Выполняем объединение. NonZero гарантирует склейку объектов в монолиты.
    clipper.Execute(Clipper2Lib::ClipType::Union, Clipper2Lib::FillRule::NonZero, polytree);

    std::vector<Loops> islands;
    islands.reserve(polytree.Count() + 1u);
    // Запускаем рекурсию от корня (Polytree root)
    gatherIslands(polytree, islands);
    return islands;
}

std::vector<Loops> intersectWithMasks(const Loops &subject, const std::vector<Loops> &masks)
{
    if (subject.empty())
    {
        return {};
    }
    // If no mask - then do Identity.
    if (masks.empty())
    {
        return {subject};
    }

    Clipper2Lib::ClipperD clipper(kPrecision);

    // 1. Добавляем Subject (наш объект) как основной контур для резки
    clipper.AddSubject(toPathsD(subject));

    // 2. Добавляем Mask (все острова маски) как Clip-объект
    // Clipper объединит все пути в AddClip автоматически перед операцией Intersection
    clipper.AddClip(toPathsD(masks));

    Clipper2Lib::PolyTreeD polytree;

    // 3. Выполняем пересечение. NonZero здесь оптимален для работы с уже разрешенной топологией.
    clipper.Execute(Clipper2Lib::ClipType::Intersection, Clipper2Lib::FillRule::NonZero, polytree);

    std::vector<Loops> result_islands;
    result_islands.reserve(polytree.Count() + 1u);
    // 4. Используем gatherIslands для восстановления структуры островов и дырок
    gatherIslands(polytree, result_islands);

    return result_islands;
}

Loops resolveEvenOddInternalTopology(Loops raw_loops)
{
    if (raw_loops.size() <= 1)
    {
        return raw_loops; // Если один контур, разбираться не с чем
    }

    // Превращаем в PathsD для Clipper
    const Clipper2Lib::PathsD paths = toPathsD(raw_loops);

    // Применяем Union с правилом EvenOdd.
    // Это превратит наложенные контуры одного элемента в "правильную" структуру:
    // Внешний будет CCW, внутренние (дырки) станут CW.
    const Clipper2Lib::PathsD resolved = Clipper2Lib::Union(paths, Clipper2Lib::FillRule::EvenOdd);

    return fromPathsD(resolved);
}

} // namespace geometry_engine
