#pragma once

#include "tess_svg/GlDefs.h"
#include "tess_svg/processing_data.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <numeric>
#include <variant>
#include <vector>

#include <gtest/gtest.h>

namespace Test {
class TestUtils
{
  public:
    // Вспомогательная структура для вычислений в тесте
    struct Point2D
    {
        double x, y;
        bool operator<(const Point2D &other) const
        {
            return x < other.x || (x == other.x && y < other.y);
        }
    };

    /**
     * @brief Calculates the area of a non-self-intersecting polygon using Shoelace Formula.
     * Works for any number of vertices defining a closed loop.
     */
    static double calculatePolygonArea(const Polyline &v)
    {
        if (v.size() < 3)
        {
            return 0.0;
        }

        double area = 0.0;
        const std::size_t n = v.size();

        for (std::size_t i = 0; i < n; ++i)
        {
            // Берем текущую точку и следующую (зацикливаем в конец, если это последняя точка)
            const auto &p1 = v[i];
            const auto &p2 = v[(i + 1) % n];
            area += (p1.x() * p2.y()) - (p2.x() * p1.y());
        }

        return area / 2.0;
    }

    static double calculateTotalWorldArea(const SvgWorld &world)
    {
        double total_area = 0.0;
        for (const auto &group : world.scene)
        {
            for (const auto &tess : group.elements)
            {
                total_area += calculatePolygonArea(tess.finalData());
            }
        }
        return total_area;
    }

    static double calculateTotalLoopsArea(const Loops &loops)
    {
        return std::accumulate(loops.begin(), loops.end(), 0.0, [](double curr, const Polyline &v) {
            return curr + calculatePolygonArea(v);
        });
    }

    // Алгоритм Эндрю для построения выпуклой оболочки
    static std::vector<Point2D> getConvexHull(std::vector<Point2D> &pts)
    {
        std::size_t n = pts.size(), k = 0;
        if (n <= 3)
        {
            return pts;
        }
        std::vector<Point2D> hull(2 * n);

        // Сортировка по X, затем по Y
        std::sort(pts.begin(), pts.end());

        // Нижняя часть оболочки
        for (std::size_t i = 0; i < n; ++i)
        {
            while (k >= 2
                   && (hull[k - 1].x - hull[k - 2].x) * (pts[i].y - hull[k - 2].y)
                          - (hull[k - 1].y - hull[k - 2].y) * (pts[i].x - hull[k - 2].x)
                        <= 0)
            {
                k--;
            }
            hull[k++] = pts[i];
        }

        // Верхняя часть оболочки
        for (std::size_t i = n, t = k + 1; i > 0; --i)
        {
            while (k >= t
                   && (hull[k - 1].x - hull[k - 2].x) * (pts[i - 1].y - hull[k - 2].y)
                          - (hull[k - 1].y - hull[k - 2].y) * (pts[i - 1].x - hull[k - 2].x)
                        <= 0)
            {
                k--;
            }
            hull[k++] = pts[i - 1];
        }

        hull.resize(k - 1);
        return hull;
    }

    static double calculateConvexHullArea(std::vector<Point2D> pts)
    {
        auto hull = getConvexHull(pts);
        double area = 0.0;
        for (size_t i = 0; i < hull.size(); ++i)
        {
            area += (hull[i].x * hull[(i + 1) % hull.size()].y)
                    - (hull[(i + 1) % hull.size()].x * hull[i].y);
        }
        return std::abs(area) / 2.0;
    }

    /// @brief Checks if Loops has holes (can be used from SvgWorldTransformers() prior
    /// finalization!).
    /// @note It will not work for non-processed single `path` with evenodd rule. It must be
    /// processed by geometry_engine::resolveEvenOddInternalTopology() which is done by
    /// unionElementsTransformer().
    static bool hasHole(const SvgWorld &world)
    {
        for (const auto &group : world.scene)
        {
            for (const auto &element : group.elements)
            {
                // Проверяем данные, если они уже превращены в Loops или Polyline
                if (std::holds_alternative<Loops>(element.data))
                {
                    const auto &loops = std::get<Loops>(element.data);

                    double totalSignedArea = 0.0;
                    double totalAbsArea = 0.0;

                    for (const auto &loop : loops)
                    {
                        // Важно: calculatePolygonArea должен возвращать ЗНАКОВУЮ площадь!
                        const double a = TestUtils::calculatePolygonArea(loop);
                        totalSignedArea += a;
                        totalAbsArea += std::abs(a);
                    }

                    // Если сумма абсолютных площадей больше, чем модуль суммы знаковых,
                    // значит внутри есть контуры с противоположным направлением обхода (дырка).
                    if (std::abs(totalAbsArea - std::abs(totalSignedArea)) > 1e-4)
                    {
                        return true;
                    }
                }
                else if (std::holds_alternative<Polyline>(element.data))
                {
                    // Если это одна полилиния, она может представлять дырку только через мостик.
                    // Но наш hasHole ориентирован на поиск топологических дырок в объекте-острове.
                    ADD_FAILURE()
                      << "You cannot test tesselated output as it does not support holes itself.";
                }
            }
        }
        return false;
    }
};

} // namespace Test
