#include "tess_svg/GlDefs.h"
#include "tess_svg/geometry_engine.hpp"
#include "tests_utils.hpp"

#include <iterator>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace Test {

class GeometryEngineTest : public ::testing::Test, public TestUtils
{
  protected:
    // Вспомогательный метод для быстрого создания Loops из списка пар (x, y)
    Loops createLoop(const std::vector<std::pair<double, double>> &points)
    {
        Loops l;
        Polyline v_list;
        for (const auto &[x, y] : points)
        {
            v_list.emplace_back(static_cast<float>(x), static_cast<float>(y));
        }
        l.emplace_back(std::move(v_list));
        return l;
    }

    void appendPolylines(Loops &appendTo, Loops what)
    {
        appendTo.insert(appendTo.end(), std::make_move_iterator(what.begin()),
                        std::make_move_iterator(what.end()));
    }

    void appendPolyline(Loops &appendTo, Polyline what)
    {
        appendTo.insert(appendTo.end(), std::move(what));
    }
};

// 1. Тест: Объединение (Union) перекрывающихся прямоугольников
TEST_F(GeometryEngineTest, UnionOverlappingRects)
{
    // Первый квадрат: (0,0) -> (2,2). Area = 4
    const Loops rect1 = createLoop({{0, 0}, {2, 0}, {2, 2}, {0, 2}});
    // Второй квадрат (перекрывает): (1,1) -> (3,3). Area = 4
    const Loops rect2 = createLoop({{1, 1}, {3, 1}, {3, 3}, {1, 3}});

    const std::vector<Loops> shapes = {rect1, rect2};
    const auto result = geometry_engine::unionShapes(shapes);
    ASSERT_EQ(result.size(), 1u);

    // Площадь объединенной фигуры должна быть меньше 8 (4+4), так как есть перекрытие.
    // Для этих двух квадратов площадь объединения будет 7.0
    const auto fullArea = calculateTotalLoopsArea(rect1) + calculateTotalLoopsArea(rect2);
    const auto expectedArea = calculateTotalLoopsArea(result.front());
    EXPECT_NEAR(expectedArea, 7.0, 1e-5);
    EXPECT_LT(expectedArea, fullArea);
    EXPECT_EQ(result.front().size(), 1u);
}

// 2. Тест: Отсечение (Intersection / Clipping)
TEST_F(GeometryEngineTest, IntersectWithMask)
{
    // Объект: Прямоугольник (0,0) -> (10,10). Area = 100
    const Loops subject = createLoop({{0, 0}, {10, 0}, {10, 10}, {0, 10}});

    // Маска: Квадрат в центре (2,2) -> (5,5). Area = 9
    const Loops mask = createLoop({{2, 2}, {5, 2}, {5, 5}, {2, 5}});

    const std::vector<Loops> masks = {mask};
    const Loops result = geometry_engine::intersectWithMasks(subject, masks);

    // Результат пересечения — это маленький квадрат (3x3) внутри. Area = 9
    const auto fullArea = calculateTotalLoopsArea(subject);
    const double area = calculateTotalLoopsArea(result);
    EXPECT_NEAR(area, 9.0, 1e-5);
    EXPECT_LT(area, fullArea);
    EXPECT_EQ(result.size(), 1u);
}

// 3. Тест: Отсечение "пустой" маской (ничего не должно измениться)
TEST_F(GeometryEngineTest, IntersectWithEmptyMasks)
{
    const Loops subject = createLoop({{0, 0}, {10, 0}, {10, 10}, {0, 10}});
    const std::vector<Loops> empty_masks;

    const Loops result = geometry_engine::intersectWithMasks(subject, empty_masks);

    EXPECT_EQ(result.size(), 1u);
    EXPECT_NEAR(calculateTotalLoopsArea(result), calculateTotalLoopsArea(subject), 1e-5);
}

// 4. Тест: Сложная форма (D-образная фигура через Union)
TEST_F(GeometryEngineTest, ComplexShapeUnion)
{
    // Прямоугольник
    const Loops rect = createLoop({{0, 0}, {10, 0}, {10, 10}, {0, 10}});
    // Добавим еще одну фигуру для проверки сложности объединения
    const Loops triangle = createLoop({{10, 0}, {15, 5}, {10, 10}});

    const std::vector<Loops> shapes = {rect, triangle};
    const auto result = geometry_engine::unionShapes(shapes);
    ASSERT_EQ(result.size(), 1u);

    // Площадь прямоугольника (100) + треугольника (25) = 125.
    // Но они имеют общую грань, Clipper должен сделать один контур без лишних линий внутри.
    EXPECT_NEAR(calculateTotalLoopsArea(result.front()),
                calculateTotalLoopsArea(rect) + calculateTotalLoopsArea(triangle), 1e-4);
    EXPECT_EQ(result.front().size(), 1u);
}

// 5. Тест: Проверка правила EvenOdd и сохранения дырок (Donut shape)
TEST_F(GeometryEngineTest, UnionEvenOddHolePreservation)
{
    /*
       Создаем два контура:
       1. Внешний квадрат (CCW): (0,0) -> (10,0) -> (10,10) -> (0,10) [Area = 100]
       2. Внутренний квадрат (CW - против часовой для создания дырки в EvenOdd):
          (2,2) -> (8,2) -> (8,8) -> (2,8) [Area = 36]
    */
    const Loops outer_loop = createLoop({{0, 0}, {10, 0}, {10, 10}, {0, 10}}); // CCW
    const Loops inner_loop =
      createLoop({{2, 8}, {8, 8}, {8, 2}, {2, 2}}); // CW (разворот для теста)

    // Мы передаем их как два отдельных объекта в списке Union
    const std::vector<Loops> shapes = {outer_loop, inner_loop};

    // Выполняем Union с нашим новым WindingRule (EvenOdd)
    const auto result = geometry_engine::unionShapes(shapes);
    ASSERT_EQ(result.size(), 1u);
    /*
       Ожидаемый результат:
       1. Количество контуров в результате должно быть равно 2 (внешний + дырка).
       2. Площадь должна быть разностью площадей (100 - 36 = 64),
          так как EvenOdd интерпретирует внутренний контур как отверстие.
    */

    // Проверка количества контуров
    EXPECT_EQ(result.front().size(), 2u)
      << "Union with EvenOdd should result in exactly two loops (outer and hole).";

    // Проверка площади
    const double totalArea = calculateTotalLoopsArea(result.front());
    EXPECT_NEAR(totalArea, 64.0, 1e-5);
}

// 6. Тест: Проверка того, что Union не "схлопывает" несвязанные объекты в один контур (Islands)
TEST_F(GeometryEngineTest, UnionSeparatedIslands)
{
    // Два квадрата далеко друг от друга
    const Loops rect1 = createLoop({{0, 0}, {2, 0}, {2, 2}, {0, 2}}); // Area 4
    const Loops rect2 = createLoop({{5, 5}, {7, 5}, {7, 7}, {5, 7}}); // Area 4

    const std::vector<Loops> shapes = {rect1, rect2};
    const auto result = geometry_engine::unionShapes(shapes);

    // Результат должен содержать 2 независимых контура (острова)
    ASSERT_EQ(result.size(), 2u);
    EXPECT_NEAR(calculateTotalLoopsArea(result[0]), 4.0, 1e-5);
    EXPECT_NEAR(calculateTotalLoopsArea(result[1]), 4.0, 1e-5);
}

// 7. Тест: Сложная топология (Остров внутри дырки) - Nesting Test
TEST_F(GeometryEngineTest, NestedIslandsTest)
{
    /*
       Создаем три контура для проверки вложенности:
       1. Внешний квадрат (Island A): (0,0) -> (20,20). Area = 400
       2. Квадрат-дырка (Hole B): (5,5) -> (15,15). Area = 100
       3. Внутренний остров (Island C) внутри дырки Б: (8,8) -> (12,12). Area = 16

        При использовании EvenOdd в Union:
        - Island A + Hole B $\rightarrow$ создают "бублик" с площадью $400 - 100 = 300$.
        - Island C находится внутри этой дырки.
     */
    Loops islandA_outer = createLoop({{0, 0}, {20, 0}, {20, 20}, {0, 20}}); // CCW
    {
        Loops holeB = createLoop({{5, 5}, {15, 5}, {15, 15}, {5, 15}}); // CW (дырка)
        appendPolylines(islandA_outer, std::move(holeB));
    }
    Loops islandC = createLoop({{8, 8}, {12, 8}, {12, 12}, {8, 12}}); // CCW

    // Важно: Мы имитируем ситуацию, когда все это — один сложный объект или группа
    const std::vector<Loops> shapes = {
      geometry_engine::resolveEvenOddInternalTopology(std::move(islandA_outer)),
      std::move(islandC)};

    // Выполняем Union с правилом EvenOdd (через твой WindingRule)
    const auto result = geometry_engine::unionShapes(shapes);

    /*
       Ожидаемый результат после восстановления:
       Мы должны получить 2 независимых элемента в векторе Loops:
       Элемент 1: {Контур А, Контур Б} -> Площадь $400 - 100 = 300$
       Элемент 2: {Контур В}          -> Площадь $16$
    */

    // Note, result[0] and result[1] maybe swapped, Clipper2 decides.
    ASSERT_EQ(result.size(), 2u) << "Should result in exactly 2 separate islands.";

    // Проверяем первый остров (бублик)
    const double areaA = calculateTotalLoopsArea(result[1]); // Должно быть ~300
    EXPECT_NEAR(areaA, 300.0, 1e-5);
    EXPECT_EQ(result[1].size(), 2u) << "First element should contain outer loop and its hole.";

    // Проверяем второй остров (маленький квадрат внутри дырки)
    const double areaC = calculateTotalLoopsArea(result[0]); // Должно быть ~16
    EXPECT_NEAR(areaC, 16.0, 1e-5);
    EXPECT_EQ(result[0].size(), 1u) << "Second element should be a single loop.";
}

} // namespace Test
