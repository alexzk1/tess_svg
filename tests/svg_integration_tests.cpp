#include "tess_svg/GlDefs.h"
#include "tess_svg/SvgProcessor.h"
#include "tess_svg/processing_data.hpp"
#include "tests_utils.hpp"

#include <cstdlib>
#include <format>
#include <numbers>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

#include <gtest/gtest.h>

// Topological Simplification Note: To ensure compatibility with downstream processors (e.g., 2D
// physics engines), internal holes are integrated into the exterior boundary during tessellation.
// This avoids topological discontinuities caused by "bridges" and ensures that each object is
// represented as a single, continuous vertex array. In this mode, disconnected voids are treated as
// non-reachable features within the primary contour.

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
    const Polyline square = {{0, 0}, {10, 0}, {10, 10}, {0, 10}};
    EXPECT_NEAR(TestUtils::calculatePolygonArea(square), 100.0, 1e-7);

    // 2. Треугольник (половина квадрата)
    const Polyline tri = {{0, 0}, {10, 0}, {0, 10}};
    EXPECT_NEAR(TestUtils::calculatePolygonArea(tri), 50.0, 1e-7);

    // 3. Сложный многоугольник (L-shape)
    const Polyline lShape = {{0, 0}, {2, 0}, {2, 1}, {1, 1}, {1, 2}, {0, 2}}; // Area = 3
    EXPECT_NEAR(TestUtils::calculatePolygonArea(lShape), 3.0, 1e-7);

    EXPECT_NEAR(TestUtils::calculateTotalLoopsArea({square}),
                TestUtils::calculatePolygonArea(square), 1e-7);
    EXPECT_NEAR(TestUtils::calculateTotalLoopsArea({square, tri, lShape}),
                TestUtils::calculatePolygonArea(square) + TestUtils::calculatePolygonArea(tri)
                  + TestUtils::calculatePolygonArea(lShape),
                1e-7);
}

TEST(MathUnitTest, HolesDetectorSelfCheck)
{
    SCOPED_TRACE("Validatating that TestUtils::hasHole() works properly.");

    struct Observation
    {
        bool holeBefore = false;
        bool holeAfter = false;
    };

    // Тестовые сценарии
    struct TestCase
    {
        std::string name;
        std::string svg;
        bool expectedHoleInInput; // Проверяем, что парсер увидел дырку в <path>
        bool singlePath;
    };

    static const std::vector<TestCase> cases = {
      // 1. Чистый квадрат: Нет дырки до -> Нет дырки после
      {"SolidSquare", R"svg(<svg><rect x="0" y="0" width="10" height="10"/></svg>)svg", false,
       false},

      // 2. Путь с дыркой: Есть дырка до (в <path>) -> Должна быть после Union
      {"PathWithHole",
       R"svg(<svg><path fill-rule="evenodd" d="M0,0 H10 V10 H0 Z M2,2 H8 V8 H2 Z"/></svg>)svg",
       true, true},

      // 3. Смешанный случай: Квадрат и треугольник.
      {"OverlappingShapesNoHole",
       R"svg(<svg><rect x="0" y="0" width="10" height="10"/><path d="M5,5 L15,5 L10,15 Z"/></svg>)svg",
       false, false}};

    for (const auto &tc : cases)
    {
        SCOPED_TRACE(tc.name);
        std::stringstream ss(tc.svg);

        Observation obs;
        auto world = loadSvgWorld(ss); // Загружаем "сырой" мир из XML

        // --- ЗАПУСК КОНВЕЙЕРА С ПЕРЕХВАТОМ СОСТОЯНИЯ ---
        std::ignore =
          SvgWorldTransformers()
            // Шаг 1: Проверяем состояние сразу после парсинга (до Union)
            .addTransformer([&](SvgWorld &w) {
                if (TestUtils::hasHole(w))
                {
                    obs.holeBefore = true;
                }
            })
            // Шаг 2: Основная бизнес-логика трансформации
            .addTransformer(&unionElementsTransformer)
            // Шаг 3: Проверяем состояние после Union, но ДО тесселяции (в формате Loops)
            .addTransformer([&](SvgWorld &w) {
                if (TestUtils::hasHole(w))
                {
                    obs.holeAfter = true;
                }
            })
            // Завершаем построение и прогоняем через всё
            .buildSurroundingPolygons(world);

        // --- АНАЛИЗ РЕЗУЛЬТАТОВ ---

        // 1. Проверяем, правильно ли мы распознали дырку в самом SVG (на этапе парсинга)
        if (tc.singlePath)
        {
            EXPECT_EQ(obs.holeBefore, false) << "Single path prior unionElementsTransformer is not "
                                                "expected to be properly winding for test math.";
        }
        else
        {
            EXPECT_EQ(obs.holeBefore, tc.expectedHoleInInput)
              << "Parser/StageA failed to detect hole in input for " << tc.name;
        }

        // 2. ПРОВЕРКА ГЛАВНОГО: Не убил ли Union нашу топологию?
        if (tc.expectedHoleInInput)
        {
            EXPECT_TRUE(obs.holeAfter)
              << "Union transformation destroyed the topological hole! For " << tc.name;
        }
        else
        {
            EXPECT_FALSE(obs.holeAfter)
              << "Union transformation created a phantom hole! For " << tc.name;
        }
    }
}
TEST(MathUnitTest, AbsoluteTopologicalVerification)
{
    const std::string svg = R"svg(
        <svg>
            <!-- Объект с дыркой -->
            <path fill-rule="evenodd" d="M0,0 H10 V10 H0 Z M2,2 H8 V8 H2 Z"/>
        </svg>)svg";

    std::stringstream ss(svg);
    auto world = loadSvgWorld(ss);

    // Применяем весь пайплайн (Stage A + Stage B)
    unionElementsTransformer(world);

    for (const auto &group : world.scene)
    {
        for (const auto &element : group.elements)
        {
            if (std::holds_alternative<Loops>(element.data))
            {
                const auto &loops = std::get<Loops>(element.data);

                // 1. Проверка через вашу функцию (проверяем логику интерфейса)
                const bool detectedByFunction =
                  TestUtils::hasHole(world); // или локально для island

                // 2. ПРОВЕРКА МАТЕМАТИЧЕСКОЙ ИСТИНЫ (Независимый метод)
                double sumSignedArea = 0.0;
                double sumAbsArea = 0.0;

                for (const auto &loop : loops)
                {
                    // Важно: используем вашу функцию, которая считает знаковую площадь!
                    const double a = TestUtils::calculatePolygonArea(loop);
                    sumSignedArea += a;
                    sumAbsArea += std::abs(a);
                }

                // Проверка на наличие дырки через математическое различие
                const bool mathematicallyHasHole =
                  (std::abs(sumAbsArea - std::abs(sumSignedArea)) > 1e-4);

                // ВОТ ЗДЕСЬ ПРОИСХОДИТ НАСТОЯЩАЯ ПРОВЕРКА:
                // Мы проверяем, что наш "детектор" совпадает с математической реальностью.
                EXPECT_EQ(detectedByFunction, mathematicallyHasHole)
                  << "Divergence between hasHole() and mathematical area calculation!";

                // И дополнительно подтверждаем результат для конкретного случая (100 - 36 = 64?)
                EXPECT_NEAR(std::abs(sumSignedArea), 64.0, 1e-4); // Пример: 100 - 16
            }
        }
    }
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

TEST_F(SvgIntegrationTest, TransformerOrderAndExecution)
{
    // Подготовим данные для проверки
    std::vector<int> call_order;
    call_order.reserve(3u);

    const auto transformer1 = [&](SvgWorld &) {
        call_order.push_back(1);
    };
    const auto transformer2 = [&](SvgWorld &) {
        call_order.push_back(2);
    };
    const auto transformer3 = [&](SvgWorld &) {
        call_order.push_back(3);
    };

    std::stringstream ss("<svg/>");
    std::ignore = SvgWorldTransformers()
                    .addTransformer(transformer1)
                    .addTransformer(transformer2)
                    .addTransformer(transformer3)
                    .buildSurroundingPolygons(loadSvgWorld(ss));

    ASSERT_EQ(call_order.size(), 3u);

    EXPECT_EQ(call_order[0], 1);
    EXPECT_EQ(call_order[1], 2);
    EXPECT_EQ(call_order[2], 3);
}

TEST_F(SvgIntegrationTest, TransformerIdentity)
{
    auto scaleTransformer = [](SvgWorld &w) {
        for (auto &group : w.scene)
        {
            for (auto &element : group.elements)
            {
                EXPECT_TRUE(!element.isEmpty());
                EXPECT_EQ(element.id(), "rect");
            }
        }
    };

    bool called = false;
    auto transformer = [&](SvgWorld &w) {
        called = true;
        ASSERT_FALSE(w.scene.empty());
        w.scene[0].id_ += "_transformed";
    };

    std::stringstream ss("<svg><rect width='10' height='10'/></svg>");
    const auto final = SvgWorldTransformers()
                         .addTransformer(scaleTransformer)
                         .addTransformer(transformer)
                         .buildSurroundingPolygons(loadSvgWorld(ss));

    EXPECT_TRUE(called);
    EXPECT_EQ(final.scene.front().id(), "rect_transformed");
    EXPECT_TRUE(final.scene.front().elements.front().isFinal());
}

TEST_F(SvgIntegrationTest, UnionElementsTransformerMergesOverlappingShapes)
{
    /*
       Тест на слияние: Два перекрывающихся квадрата.
       Square 1: (0,0) to (20,20) -> Area = 400
       Square 2: (10,0) to (30,20) -> Area = 400
       Intersection area is (10 * 20) = 200.
       Total Union Area should be: 400 + 400 - 200 = 600.
       Если слияние НЕ работает, сумма площадей будет 800 (или около того).
    */
    const std::string svg = R"svg(
        <svg>
            <rect x="0" y="0" width="20" height="20"/>
            <rect x="10" y="0" width="20" height="20"/>
        </svg>)svg";

    std::stringstream ss(svg);
    const auto final_world = SvgWorldTransformers()
                               .addTransformer(&unionElementsTransformer)
                               .buildSurroundingPolygons(loadSvgWorld(ss));

    EXPECT_NEAR(calculateTotalWorldArea(final_world), 600.0, 1e-4);
}

TEST_F(SvgIntegrationTest, UnionElementsTransformerSimplifiesDisjointIslands)
{
    /*
       Тест на сохранение раздельных объектов:
       Два далеких друг от друга квадрата не должны слиться в один контур (по площади),
       но они должны быть обработаны как отдельные элементы.
    */
    const std::string svg = R"svg(
        <svg>
            <rect x="0" y="0" width="10" height="10"/>   <!-- Area 100 -->
            <rect x="100" y="100" width="10" height="10"/> <!-- Area 100 -->
        </svg>)svg";

    std::stringstream ss(svg);
    const auto final_world = SvgWorldTransformers()
                               .addTransformer(&unionElementsTransformer)
                               .addTransformer([](SvgWorld &w) {
                                   EXPECT_FALSE(hasHole(w));
                               })
                               .buildSurroundingPolygons(loadSvgWorld(ss));

    EXPECT_NEAR(calculateTotalWorldArea(final_world), 200.0, 1e-4);
}

TEST_F(SvgIntegrationTest, OuterBoundaryIntegrityWithInternalHoles)
{
    /*
       Тест: Проверка целостности внешней границы (Outer Hull).
       Мы создаем L-образную фигуру с дыркой внутри.
       L-shape координаты: (0,0), (10,0), (10,5), (5,5), (5,10), (0,10)
       Внутренняя дырка: круг или квадрат в центре.

        Математика:
        Площадь L-shape = 75.
        Выпуклая оболочка (Convex Hull) этого L-shape — это прямоугольник (0,0) to (10,10).
        Area Convex Hull = 100.

         Если тесселятор из-за дырки или мостиков исказит хотя бы одну крайнюю точку
         в сторону центра, площадь выпуклой оболочки станет < 100.
      */
    const std::string svg = R"svg(
       <svg>
        <path fill-rule="evenodd" d="M 0 0 L 10 0 L 10 5 L 5 5 L 5 10 L 0 10 Z M 2 2 L 4 2 L 3 5 Z" />
       </svg>)svg)svg";

    /*
     (0⋅0)−(0⋅10)=0(0⋅0)−(0⋅10)=0
     (10⋅5)−(0⋅10)=50(10⋅5)−(0⋅10)=50
     (10⋅10)−(5⋅5)=75(10⋅10)−(5⋅5)=75
     (5⋅10)−(10⋅0)=50(5⋅10)−(10⋅0)=50
     (0⋅0)−(10⋅0)=0(0⋅0)−(10⋅0)=0

  Сумма =0+50+75+50+0=175=0+50+75+50+0=175.
  Площадь =175/2=87.5=175/2=87.5.
     */
    const double expectedHullArea = 87.5;

    std::stringstream ss(svg);
    const auto final_world = SvgWorldTransformers()
                               .addTransformer(&unionElementsTransformer)
                               .addTransformer([](SvgWorld &w) {
                                   EXPECT_TRUE(hasHole(w)) << "Check test setup, hole is missing.";
                               })
                               .buildSurroundingPolygons(loadSvgWorld(ss));

    // 1. Собираем все точки из всех полученных элементов (Islands)
    std::vector<TestUtils::Point2D> all_points;
    for (const auto &group : final_world.scene)
    {
        for (const auto &element : group.elements)
        {
            // Используем ваш метод доступа к данным (get if Polyline)
            if (!element.isEmpty() && std::holds_alternative<Polyline>(element.data))
            {
                const auto &pl = std::get<Polyline>(element.data);
                for (const auto &v : pl)
                {
                    all_points.push_back({static_cast<double>(v.x()), static_cast<double>(v.y())});
                }
            }
        }
    }

    ASSERT_FALSE(all_points.empty()) << "No vertices found after transformation!";

    // 2. Считаем площадь выпуклой оболочки всех точек
    const double actualHullArea = TestUtils::calculateConvexHullArea(all_points);

    // Проверяем, что внешняя граница не была стянута внутрь (допустимая погрешность на float)
    EXPECT_NEAR(actualHullArea, expectedHullArea, 1e-4);
}

TEST_F(SvgIntegrationTest, OuterBoundaryIntegrityWithInternalTriangularHole)
{
    /*
       Тест: "Квадрат с треугольной дыркой".
       Внешний контур (Square): (0,0), (10,0), (10,10), (0,10). Area = 100.
       Внутренний контур (Triangle Hole): (2,2), (4,2), (3,5).

 Ожидаемый Convex Hull: Квадрат (0,0) to (10,10). Area = 100.
 Этот тест проверяет, что никакие "мостики" или операции Clipper/GLU
 не изменили положение внешних вершин и не создали новых крайних точек.
*/
    const std::string svg = R"svg(<svg>
            <path fill-rule="evenodd" d="M0,0 L10,0 L10,10 L0,10 Z M2,2 L4,2 L3,5 Z"/>
        </svg>)svg";

    const double expectedHullArea = 100.0;

    std::stringstream ss(svg);
    // Прогоняем через весь пайплайн до финальных полилиний
    const auto final_world = SvgWorldTransformers()
                               .addTransformer(&unionElementsTransformer)
                               .addTransformer([](SvgWorld &w) {
                                   EXPECT_TRUE(hasHole(w)) << "Check test setup, hole is missing.";
                               })
                               .buildSurroundingPolygons(loadSvgWorld(ss));

    // 1. Собираем все вершины из всех получившихся островов/элементов
    std::vector<TestUtils::Point2D> all_points;
    for (const auto &group : final_world.scene)
    {
        for (const auto &element : group.elements)
        {
            if (!element.isEmpty() && std::holds_alternative<Polyline>(element.data))
            {
                const auto &pl = std::get<Polyline>(element.data);
                for (const auto &v : pl)
                {
                    all_points.push_back({static_cast<double>(v.x()), static_cast<double>(v.y())});
                }
            }
            else if (!element.isEmpty() && std::holds_alternative<Loops>(element.data))
            {
                // Если по какой-то причине тесселятор вернул Loops (что маловероятно для
                // финализатора)
                const auto &loops = std::get<Loops>(element.data);
                for (const auto &pl : loops)
                {
                    for (const auto &v : pl)
                    {
                        all_points.push_back(
                          {static_cast<double>(v.x()), static_cast<double>(v.y())});
                    }
                }
            }
        }
    }

    ASSERT_FALSE(all_points.empty()) << "No vertices found!";

    // 2. Считаем площадь выпуклой оболочки (Convex Hull)
    const double actualHullArea = TestUtils::calculateConvexHullArea(all_points);

    /*
       Математическое ожидание: 100.0.
       Если результат будет < 100, значит мостик или дырка "прорезали" внешний контур.
       Если результат > 100, значит возникло паразитное расширение геометрии.
    */
    EXPECT_NEAR(actualHullArea, expectedHullArea, 1e-4);
}

} // namespace Test
