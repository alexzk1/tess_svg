#include "tess_svg/GlDefs.h"
#include "tess_svg/SvgProcessor.h"
#include "tess_svg/processing_data.hpp"
#include "tests_utils.hpp"

#include <numbers>
#include <sstream>
#include <string>
#include <utility>

#include <gtest/gtest.h>

namespace Test {

class SvgIntegrationTest : public ::testing::Test, public TestUtils
{
  protected:
    // Creates world without additional processing added, plain svg->finalize.
    SvgWorld createPureWorld(const std::string &svg_content)
    {
        std::stringstream ss(svg_content);
        return SvgWorldTransformers().buildSurroundingPolygons(loadSvgWorld(ss));
    }

    // Creates world out of <defs> tag which allows to use the same test functions.
    SvgWorld createWorldFromDefs(const std::string &svg_content)
    {
        std::stringstream ss(svg_content);
        SvgWorld tmp = loadSvgWorld(ss);
        return SvgWorldTransformers().buildSurroundingPolygons({
          .scene = std::move(tmp.defs),
          .defs = {},
        });
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
    // Площадь должна быть 200
    EXPECT_NEAR(calculateTotalWorldArea(createPureWorld(svg)), 200.0, 1e-4);
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
    EXPECT_NEAR(calculateTotalWorldArea(createPureWorld(svg)), 400.0, 1e-4);
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
    // Площадь не должна измениться от поворота и переноса, только масштаб (который 1)
    EXPECT_NEAR(calculateTotalWorldArea(createPureWorld(svg)), 200.0, 1e-4);
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
    // Area 200 * (scale 2)^2 = 800. Поворот не влияет на площадь.
    EXPECT_NEAR(TestUtils::calculateTotalWorldArea(createPureWorld(svg)), 800.0, 1e-4);
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
    EXPECT_NEAR(calculateTotalWorldArea(createPureWorld(svg)), 400.0, 1e-4);
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
    EXPECT_NEAR(calculateTotalWorldArea(createPureWorld(svg)), 625.0, 1e-4);
}

TEST_F(SvgIntegrationTest, DefsSimpleShapes)
{
    const std::string svg =
      R"svg(<svg><defs><rect id="d1" width="10" height="20"/><circle cx="5" cy="5" r="5"/></defs></svg>)svg";

    // Rect: 10*20 = 200. Circle: pi * 5^2 approx 78.5398
    EXPECT_NEAR(calculateTotalWorldArea(createWorldFromDefs(svg)),
                200.0 + (std::numbers::pi * 25.0), 1e-2);
}

TEST_F(SvgIntegrationTest, DefsWithTransformations)
{
    const std::string svg = R"svg(
        <svg>
            <defs>
                <g transform="scale(2)">
                    <rect width="10" height="10"/> <!-- Area 100 -> 400 -->
                </g>
                <g transform="rotate(90) translate(5,0)">
                    <rect width="10" height= "5"/> <!-- Area 50. Rotate/Translate don't change area -->
                </g>
            </defs>
        </svg>)svg";

    // Ожидаем: (10*10 * 2^2) + (10*5) = 400 + 50 = 450
    EXPECT_NEAR(calculateTotalWorldArea(createWorldFromDefs(svg)), 450.0, 1e-4);
}

// 3. Тест: Изоляция трансформаций (Scene vs Defs)
TEST_F(SvgIntegrationTest, DefsIsolationFromScene)
{
    const std::string svg = R"svg(
        <svg>
            <!-- Группа в сцене масштабируется -->
            <g transform="scale(10)">
                <rect width="2" height="2"/> <!-- Area 4 -> 400 -->
            </g>
            <!-- Определения остаются маленькими и независимыми -->
            <defs>
                <rect id="d1" width="5" height="5"/> <!-- Area 25 -->
            </defs>
        </svg>)svg";

    // Проверяем, что масштаб сцены не "протек" в defs
    EXPECT_NEAR(calculateTotalWorldArea(createPureWorld(svg)), 400.0, 1e-4);
    EXPECT_NEAR(calculateTotalWorldArea(createWorldFromDefs(svg)), 25.0, 1e-4);
}

// 4. Тест: Глубокая вложенность внутри defs (проверка рекурсии)
TEST_F(SvgIntegrationTest, DeeplyNestedDefs)
{
    const std::string svg = R"svg(
        <svg>
            <defs>
                <g transform="scale(2)">
                    <g transform="translate(10, 10)">
                        <circle r="5"/> <!-- Area pi*25 approx 78.5 -->
                    </g>
                </g>
            </defs>
        </svg>)svg";

    // Area: (pi * 25) * scale(2)^2 = pi * 100 approx 314.159
    EXPECT_NEAR(calculateTotalWorldArea(createWorldFromDefs(svg)), std::numbers::pi * 100.0, 1e-2);
}

} // namespace Test
