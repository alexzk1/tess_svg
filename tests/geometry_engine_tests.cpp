#include "tess_svg/GlDefs.h"
#include "tess_svg/geometry_engine.hpp"
#include "tess_svg/util_helpers.h"
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
        appendVectors(appendTo, std::move(what));
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
    const auto result = geometry_engine::intersectWithMasks(subject, masks);
    ASSERT_EQ(result.size(), 1u);

    // Результат пересечения — это маленький квадрат (3x3) внутри. Area = 9
    const auto fullArea = calculateTotalLoopsArea(subject);
    const double area = calculateTotalLoopsArea(result.front());
    EXPECT_NEAR(area, 9.0, 1e-5);
    EXPECT_LT(area, fullArea);
    EXPECT_EQ(result.size(), 1u);
}

// 3. Тест: Отсечение "пустой" маской (ничего не должно измениться)
TEST_F(GeometryEngineTest, IntersectWithEmptyMasks)
{
    const Loops subject = createLoop({{0, 0}, {10, 0}, {10, 10}, {0, 10}});
    const std::vector<Loops> empty_masks;

    const auto result = geometry_engine::intersectWithMasks(subject, empty_masks);
    ASSERT_EQ(result.size(), 1u);

    EXPECT_EQ(result.front().size(), 1u);
    EXPECT_NEAR(calculateTotalLoopsArea(result.front()), calculateTotalLoopsArea(subject), 1e-5);
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

// 8. Тест: Пересечение с маской, состоящей из двух раздельных островов (Split Test)
// Мы проверяем, что если объект пересекается с двумя разрозненными областями,
// мы получаем два отдельных острова в векторе Loops.
TEST_F(GeometryEngineTest, IntersectWithMultiIslandMaskSplitsObject)
{
    // Объект: Большой прямоугольник (0, 0) -> (10, 10). Area = 100
    const Loops subject = createLoop({{0, 0}, {10, 0}, {10, 10}, {0, 10}});

    // Маска: Два маленьких квадрата внутри большого rectangle.
    // Первый: (1,1) -> (2,2). Area = 1
    // Второй: (8,8) -> (9,9). Area = 1
    const Loops mask_island1 = createLoop({{1, 1}, {2, 1}, {2, 2}, {1, 2}});
    const Loops mask_island2 = createLoop({{8, 8}, {9, 8}, {9, 9}, {8, 9}});

    // В Clipper2 Intersection с несколькими объектами в AddClip
    // даст два раздельных острова. Мы должны получить std::vector из 2-х Loops.
    const std::vector<Loops> masks = {mask_island1, mask_island2};

    const auto result = geometry_engine::intersectWithMasks(subject, masks);

    // Проверяем количество островов (должно быть 2)
    ASSERT_EQ(result.size(), 2u);

    // Проверяем площади каждого острова
    EXPECT_NEAR(calculateTotalLoopsArea(result[0]), 1.0, 1e-5);
    EXPECT_NEAR(calculateTotalLoopsArea(result[1]), 1.0, 1e-5);
}

// 9. Тест: Пересечение "Бублика" с прямоугольником (Hole Preservation in Intersection)
TEST_F(GeometryEngineTest, IntersectWithDonutMaskPreservesHoles)
{
    /*
       Маска: Бублик (Island with a hole).
       Outer loop: (0,0) -> (10,10). Area = 100.
       Inner hole: (4,4) -> (6,6). Area = 4.
    */
    Loops donut;
    appendPolylines(donut, createLoop({{0, 0}, {10, 0}, {10, 10}, {0, 10}})); // CCW
    appendPolylines(donut, createLoop({{4, 5}, {6, 5}, {6, 7}, {4, 7}}));     // CW (дырка)
    const std::vector<Loops> mask = {geometry_engine::resolveEvenOddInternalTopology(donut)};

    // Объект для пересечения: Прямоугольник, который полностью покрывает бублик
    const Loops subject = createLoop({{-1, -1}, {11, -1}, {11, 11}, {-1, 11}});

    const auto result = geometry_engine::intersectWithMasks(subject, mask);

    // Ожидаемый результат: Один остров (Loops), содержащий 2 контура (внешний и дырку)
    ASSERT_EQ(result.size(), 1u) << "Intersection should return exactly one island.";
    EXPECT_EQ(result[0].size(), 2u) << "The resulting island must still contain its hole.";

    // Площадь должна быть Area(outer) - Area(hole) = 100 - 4 = 96
    EXPECT_NEAR(calculateTotalLoopsArea(result[0]), 96, 1e-5);
}

// 10. Тест: Маска больше объекта и полностью его перекрывает (Oversized Mask)
TEST_F(GeometryEngineTest, IntersectWithOversizedMaskDoesNotChangeSubject)
{
    // Объект: Квадрат в центре (5,5) -> (15,15). Area = 100.
    const Loops subject = createLoop({{5, 5}, {15, 5}, {15, 15}, {5, 15}});

    // Маска: Огромный прямоугольник (-100,-100) -> (200,200).
    const Loops mask = createLoop({{-100, -100}, {200, -100}, {200, 200}, {-100, 200}});

    const std::vector<Loops> masks = {mask};
    const auto result = geometry_engine::intersectWithMasks(subject, masks);

    // Результат должен быть идентичен исходному субъекту.
    ASSERT_EQ(result.size(), 1u) << "Should still be one island.";
    EXPECT_NEAR(calculateTotalLoopsArea(result[0]), 100.0, 1e-5);

    // Важнейшая проверка: ни одна точка не должна выйти за границы [5, 15]
    for (const auto &polyline : result[0])
    {
        for (const auto &v : polyline)
        {
            EXPECT_GE(v.x(), 4.99); // Используем эпсилон для float/double
            EXPECT_LE(v.x(), 15.01);
            EXPECT_GE(v.y(), 4.99);
            EXPECT_LE(v.y(), 15.01);
        }
    }
}

// 11. Тест: Маска разрезает объект пополам (Boundary/Edge Clipping)
TEST_F(GeometryEngineTest, IntersectWithHalfMaskSplitsSubject)
{
    // Объект: Квадрат (0,0) -> (10,10). Area = 100.
    const Loops subject = createLoop({{0, 0}, {10, 0}, {10, 10}, {0, 10}});

    // Маска: Прямоугольник, который закрывает только левую половину (от -5 до 5 по X)
    const Loops mask = createLoop({{-5, -5}, {5, -5}, {5, 15}, {-5, 15}});

    const std::vector<Loops> masks = {mask};
    const auto result = geometry_engine::intersectWithMasks(subject, masks);

    // Ожидаемый результат: Прямоугольник (0,0) -> (5,10). Area = 50.
    ASSERT_EQ(result.size(), 1u) << "Should produce exactly one rectangle.";
    EXPECT_NEAR(calculateTotalLoopsArea(result[0]), 50.0, 1e-5);

    // Проверяем границы результата: он должен быть ограничен X=5 и Y={0..10}
    for (const auto &polyline : result[0])
    {
        for (const auto &v : polyline)
        {
            EXPECT_GE(v.x(), -0.01); // Граница маски по X может чуть сместиться из-за precision
            EXPECT_LE(v.x(), 5.01);  // Но не должна выходить дальше линии реза!
        }
    }
}

// 12. Тест: Проверка на точность при очень маленьких пересечениях (Precision Test)
TEST_F(GeometryEngineTest, IntersectWithTinyOverlap)
{
    const Loops subject = createLoop({{0, 0}, {10, 0}, {10, 10}, {0, 10}});

    // Маска: Тонкая полоска (9.9, -1) -> (10.1, 2). Пересечение — крошечный прямоугольник (9.9, 0)
    // -> (10, 2). Area = 0.4
    const Loops mask = createLoop({{9.9, -1}, {10.1, -1}, {10.1, 2}, {9.9, 2}});

    const std::vector<Loops> masks = {mask};
    const auto result = geometry_engine::intersectWithMasks(subject, masks);

    ASSERT_FALSE(result.empty());
    // Проверяем площадь крошечного сегмента
    EXPECT_NEAR(calculateTotalLoopsArea(result[0]), 2 * (10 - 9.9), 1e-4);
    // Rect intersection bounds: X from max(0, 9.9)=9.9 to min(10, 10.1)=10 -> width 0.1.
    // Y from max(0, -1)=0 to min(10, 2)=2 -> height 2. Area = 0.2.
    EXPECT_NEAR(calculateTotalLoopsArea(result[0]), 0.2, 1e-4);
}

} // namespace Test
