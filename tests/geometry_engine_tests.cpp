#include "tess_svg/GlDefs.h"
#include "tess_svg/geometry_engine.hpp"
#include "tests_utils.hpp"

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
        Vertexes v_list;
        for (const auto &[x, y] : points)
        {
            v_list.emplace_back(static_cast<float>(x), static_cast<float>(y));
        }
        l.push_back(std::move(v_list));
        return l;
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
    const Loops result = geometry_engine::unionShapes(shapes);

    // Площадь объединенной фигуры должна быть меньше 8 (4+4), так как есть перекрытие.
    // Для этих двух квадратов площадь объединения будет 7.0
    const auto fullArea = calculateTotalLoopsArea(rect1) + calculateTotalLoopsArea(rect2);
    const auto expectedArea = calculateTotalLoopsArea(result);
    EXPECT_NEAR(expectedArea, 7.0, 1e-5);
    EXPECT_LT(expectedArea, fullArea);
    EXPECT_EQ(result.size(), 1u);
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
    const Loops result = geometry_engine::unionShapes(shapes);

    // Площадь прямоугольника (100) + треугольника (25) = 125.
    // Но они имеют общую грань, Clipper должен сделать один контур без лишних линий внутри.
    EXPECT_NEAR(calculateTotalLoopsArea(result),
                calculateTotalLoopsArea(rect) + calculateTotalLoopsArea(triangle), 1e-4);
    EXPECT_EQ(result.size(), 1u);
}

} // namespace Test
