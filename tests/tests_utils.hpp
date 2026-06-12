#pragma once

#include "tess_svg/GlDefs.h"
#include "tess_svg/SvgProcessor.h"

#include <cstddef>
#include <cstdlib>

namespace Test {
class TestUtils
{
  public:
    /**
     * @brief Calculates the area of a non-self-intersecting polygon using Shoelace Formula.
     * Works for any number of vertices defining a closed loop.
     */
    static double calculatePolygonArea(const Vertexes &v)
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

        return std::abs(area) / 2.0;
    }

    static double calculateTotalAreaFromProcessor(const SvgProcessor &processor)
    {
        double total_area = 0.0;
        for (auto const &[group_id, group] : processor.getTesselated().scene)
        {
            for (const auto &tess : group)
            {
                total_area += calculatePolygonArea(tess.finalData());
            }
        }
        return total_area;
    }
};

} // namespace Test
