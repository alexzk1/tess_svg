#include "tess_svg/GlDefs.h"
#include "tess_svg/Tesselate.h"

#include <cstddef>

#include <gtest/gtest.h>

namespace Test {

TEST(TesselateTest, HoleTest)
{
    Tesselate tess;
    Loops loops;

    // Внешний квадрат (0,0) - (10,10)
    const Vertexes outer = {{0, 0}, {10, 0}, {10, 10}, {0, 10}};
    // Внутренний квадрат (дырка) (2,2) - (8,8)
    // Важно: для NONZERO правила часто лучше менять направление обхода
    const Vertexes inner = {{2, 2}, {2, 8}, {8, 8}, {8, 2}};

    loops.push_back(outer);
    loops.push_back(inner);

    const auto &result = tess.process(loops, false);

    // В простом квадрате без дырки было бы 2 треугольника (6 вершин)
    // С дыркой их должно быть значительно больше
    EXPECT_GT(result.size(), 6);
    // Проверка на четность (каждый треугольник - 3 вершины)
    EXPECT_EQ(result.size() % 3, 0);

    double totalArea = 0;
    for (std::size_t i = 0; i < result.size(); i += 3)
    {
        // Формула площади треугольника через координаты вершин (cross product)
        const auto &v1 = result[i];
        const auto &v2 = result[i + 1];
        const auto &v3 = result[i + 2];
        totalArea += std::abs(
          (v1.x() * (v2.y() - v3.y()) + v2.x() * (v3.y() - v1.y()) + v3.x() * (v1.y() - v2.y()))
          / 2.0);
    }
    EXPECT_NEAR(totalArea, 64.0, 1e-7);
}

TEST(TesselateTest, SelfIntersectionTest)
{
    Tesselate tess;
    Loops loops;

    // Фигура "бабочка" или "песочные часы"
    // (0,0) -> (10,10) -> (10,0) -> (0,10)
    // Пересечение должно создаться в (5,5)
    const Vertexes hourglass = {{0, 0}, {10, 10}, {10, 0}, {0, 10}};
    loops.push_back(hourglass);

    // Если l_combine работает, этот вызов не бросит исключение
    EXPECT_NO_THROW({
        const auto &result = tess.process(loops, false);
        EXPECT_FALSE(result.empty());
    });
}

TEST(TesselateTest, DegenerateTest)
{
    Tesselate tess;
    Loops loops;
    // Линия вместо полигона (все точки на одной прямой)
    const Vertexes line = {{0, 0}, {10, 10}, {5, 5}};
    loops.push_back(line);

    // Ожидаем, что тесселятор просто не создаст треугольников (площадь 0),
    // но при этом не вылетит с ошибкой.
    const auto &result = tess.process(loops, false);
    EXPECT_TRUE(result.empty() || result.size() < 3);
}

TEST(TesselateTest, BoundaryClosureTest)
{
    Tesselate tess;
    Loops loops;

    // Квадрат (0,0) - (10,10)
    const Vertexes shape = {{0, 0}, {10, 0}, {10, 10}, {0, 10}};
    loops.push_back(shape);

    // Режим контуров (Boundary Only)
    const auto &result = tess.process(loops, true);

    // Для одного квадрата GLU должен вернуть те же 4 точки (или 5, если замыкает)
    // Но в твоем коде l_vertex просто пушит точки в tlist.
    // Если GL_LINE_LOOP отработал, tlist должен содержать последовательность.

    ASSERT_GE(result.size(), 3);

    // Проверяем "физическую" замкнутость:
    // Если мы используем эти точки для EdgeChain,
    // последняя точка должна позволять провести линию к первой.
    // В случае с GLU_TESS_BOUNDARY_ONLY он выдает вершины контуров по очереди.

    // Давай проверим, что результат не пустой и количество точек логично
    EXPECT_EQ(result.size(), 4); // Ожидаем 4 вершины для квадрата
}

TEST(TesselateTest, SelfIntersectingBoundaryTest)
{
    Tesselate tess;
    Loops loops;
    // Песочные часы
    const Vertexes hourglass = {{0, 0}, {10, 10}, {10, 0}, {0, 10}};
    loops.push_back(hourglass);

    const auto &result = tess.process(loops, true);

    // Если GLU разрезал самопересечение, точек станет 6
    // (две петли по 3 точки: верхний и нижний треугольники)
    // Вместо исходных 4-х.
    EXPECT_EQ(result.size(), 6);
}

} // namespace Test
