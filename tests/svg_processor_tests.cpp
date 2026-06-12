#include "tess_svg/GlDefs.h"
#include "tess_svg/SvgProcessor.h"

#include <cmath>
#include <cstddef>
#include <memory>
#include <sstream>
#include <string>

#include <gtest/gtest.h>

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
        for (auto const &[group_id, group] : processor.getTesselated())
        {
            for (const auto &tess : group)
            {
                total_area += calculatePolygonArea(tess.vertexes);
            }
        }
        return total_area;
    }
};

class SvgIntegrationTest : public ::testing::Test, public TestUtils
{
  protected:
    std::unique_ptr<SvgProcessor> createProcessor(const std::string &svg_content)
    {
        auto ss = std::make_unique<std::stringstream>(svg_content);
        return std::make_unique<SvgProcessor>(*ss);
    }
};

TEST(MathUnitTest, PolygonShoelaceFormula)
{
    SCOPED_TRACE("Self test for the class TestUtils.");
    // 1. Квадрат 10x10 через Shoelace
    const Vertexes square = {{0, 0}, {10, 0}, {10, 10}, {0, 10}};
    EXPECT_NEAR(TestUtils::calculatePolygonArea(square), 100.0, 1e-7);

    // 2. Треугольник (половина квадрата)
    const Vertexes tri = {{0, 0}, {10, 0}, {0, 10}};
    EXPECT_NEAR(TestUtils::calculatePolygonArea(tri), 50.0, 1e-7);

    // 3. Сложный многоугольник (L-shape)
    const Vertexes lShape = {{0, 0}, {2, 0}, {2, 1}, {1, 1}, {1, 2}, {0, 2}}; // Area = 3
    EXPECT_NEAR(TestUtils::calculatePolygonArea(lShape), 3.0, 1e-7);
}

// 1. Тест: Базовый примитив (проверка, что корень-контейнер <svg> не теряет детей)
TEST_F(SvgIntegrationTest, SinglePrimitiveAtRoot)
{
    const std::string svg = R"(<svg><rect x="0" y="0" width="10" height="20"/></svg>)";
    auto processor = createProcessor(svg);

    // Площадь должна быть 200
    EXPECT_NEAR(calculateTotalAreaFromProcessor(*processor), 200.0, 1e-4);
}

// 2. Тест: Масштабирование группы (проверка накопления матриц)
TEST_F(SvgIntegrationTest, NestedScalingTransform)
{
    // Группа масштаба 2. Прямоугольник 10x10 станет 20x20 = 400 area
    const std::string svg = R"svg(
        <svg>
            <g transform="scale(2)">
                <rect x="0" y="0" width="10" height="10"/>
            </g>
        </svg>)svg";
    auto processor = createProcessor(svg);

    EXPECT_NEAR(calculateTotalAreaFromProcessor(*processor), 400.0, 1e-4);
}

// 3. Тест: Сложная вложенность (Матрица: Translate -> Rotate -> Scale)
TEST_F(SvgIntegrationTest, DeeplyNestedTransformations)
{
    /*
       Логика:
       <g transform="translate(50, 50)">   -> Смещение
         <g transform="rotate(90)">        -> Поворот на месте (относительно родителя)
           <rect x="0" y="0" width="10" height="20"/>  -> Area = 200. Scale = 1.
         </g>
       </g>
    */
    const std::string svg = R"svg(
        <svg>
            <g transform="translate(50, 50)">
                <g transform="rotate(90)">
                    <rect x="0" y="0" width="10" height="20"/>
                </g>
            </g>
        </svg>)svg";
    auto processor = createProcessor(svg);

    // Площадь не должна измениться от поворота и переноса, только масштаб (который 1)
    EXPECT_NEAR(calculateTotalAreaFromProcessor(*processor), 200.0, 1e-4);
}

TEST_F(SvgIntegrationTest, DeeplyNestedTransformationsScaled)
{
    // Проверяем масштаб и поворот одновременно
    const std::string svg = R"svg(
        <svg>
            <g transform="scale(2)">
                <g transform="rotate(90)">
                    <rect x="0" y="0" width="10" height= "20"/>
                </g>
            </g>
        </svg>)svg";
    auto ss = std::make_unique<std::stringstream>(svg);
    const SvgProcessor processor(*ss);

    // Area 200 * (scale 2)^2 = 800. Поворот не влияет на площадь.
    EXPECT_NEAR(TestUtils::calculateTotalAreaFromProcessor(processor), 800.0, 1e-4);
}

// 4. Тест: Комбинированный Scale + Translate (проверка порядка умножения матриц)
TEST_F(SvgIntegrationTest, ScaleAndTranslateOrder)
{
    /*
       Важно проверить порядок: M = M_parent * M_local
       Если parent=scale(2), child=translate(10):
       Точка (5,0) -> scale(2) -> (10,0) -> translate(10) -> (20,0).
    */
    const std::string svg = R"svg(
        <svg>
            <g transform="scale(2)">
                <rect x="5" y="0" width="10" height="10"/>
            </g>
        </svg>)svg";
    // rect (5,0) -> scale(2) -> (10,0). Размер остается 10x10, но координаты уплывают.
    // Площадь должна быть просто Area * Scale^2 = 100 * 4 = 400.

    auto processor = createProcessor(svg);
    EXPECT_NEAR(calculateTotalAreaFromProcessor(*processor), 400.0, 1e-4);
}

// 5. Тест: Несколько групп на одном уровне (проверка независимости RecursionParameters)
TEST_F(SvgIntegrationTest, SiblingsIndependence)
{
    const std::string svg = R"svg(
        <svg>
            <g transform="scale(2)"> <rect x="0" y="0" width="10" height="10"/> </g>
            <g transform="scale(3)"> <rect x="0" y="0" width="5" height="5"/>   </g>
        </svg>)svg";
    // Group 1: area = 100 * 4 = 400
    // Group 2: area = 25 * 9 = 225
    // Total = 625

    auto processor = createProcessor(svg);
    EXPECT_NEAR(calculateTotalAreaFromProcessor(*processor), 625.0, 1e-4);
}
} // namespace Test
