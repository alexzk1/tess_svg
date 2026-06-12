#include "tess_svg/GlDefs.h"
#include "tess_svg/SvgParsers.h"
#include "tess_svg/Tesselate.h"
#include "tess_svg/node_transform.hpp"
#include "tests_utils.hpp"

#include <pugixml.hpp>

#include <cstddef>
#include <cstdlib>
#include <string>

#include <gtest/gtest.h>

namespace Test {

// Структура параметров для теста
struct TestParam
{
    TessMode mode;
    std::string description; // Для понятных ошибок в консоли
};

class TesselationIntegrationTest : public ::testing::TestWithParam<TestParam>, public TestUtils
{
  protected:
    pugi::xml_document doc;

    // Вспомогательная функция для создания матрицы Identity
    GlVertex::trans_matrix_t identity()
    {
        return GlVertex::getIdentity();
    }
};

TEST_F(TesselationIntegrationTest, BoundaryClosureTest)
{
    Tesselate tess;
    Loops loops;

    // Квадрат (0,0) - (10,10)
    const Vertexes shape = {{0, 0}, {10, 0}, {10, 10}, {0, 10}};
    loops.push_back(shape);

    // Режим контуров (Boundary Only)
    const auto &result = tess.process(loops, TessMode::Contours);
    ASSERT_GE(result.size(), 3);
    EXPECT_EQ(result.size(), 4); // Ожидаем 4 вершины для квадрата
}

TEST_F(TesselationIntegrationTest, SelfIntersectingBoundaryTest)
{
    Tesselate tess;
    Loops loops;
    // Песочные часы
    const Vertexes hourglass = {{0, 0}, {10, 10}, {10, 0}, {0, 10}};
    loops.push_back(hourglass);

    const auto &result = tess.process(loops, TessMode::Contours);

    // Если GLU разрезал самопересечение, точек станет 6
    // (две петли по 3 точки: верхний и нижний треугольники)
    // Вместо исходных 4-х.
    EXPECT_EQ(result.size(), 6);
}

TEST_F(TesselationIntegrationTest, HoleTest)
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

    const auto &result = tess.process(loops, TessMode::Triangles);

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

TEST_P(TesselationIntegrationTest, SelfIntersectionTest)
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
        const auto &result = tess.process(loops, GetParam().mode);
        EXPECT_FALSE(result.empty());
    });
}

TEST_P(TesselationIntegrationTest, DegenerateTest)
{
    Tesselate tess;
    Loops loops;
    // Линия вместо полигона (все точки на одной прямой)
    const Vertexes line = {{0, 0}, {10, 10}, {5, 5}};
    loops.push_back(line);

    // Ожидаем, что тесселятор просто не создаст треугольников (площадь 0),
    // но при этом не вылетит с ошибкой.
    const auto &result = tess.process(loops, GetParam().mode);
    EXPECT_TRUE(result.empty() || result.size() < 3);
}

TEST_P(TesselationIntegrationTest, CompareParserVsManual)
{
    // 1. Подготовка данных для "песочных часов" (самопересечение - сложный кейс)
    const std::string polygonCoords = "0,0 10,10 10,0 0,10";

    // --- МЕТОД А: Ручное создание Loops (Manual) ---
    const Vertexes hourglass_manual = {{0.0f, 0.0f}, {10.0f, 10.0f}, {10.0f, 0.0f}, {0.0f, 10.0f}};

    Tesselate tess;
    auto result_manual = tess.process({hourglass_manual}, GetParam().mode);
    const double area_manual = calculatePolygonArea(result_manual);

    // --- МЕТОД Б: Через SvgParsers (Parser-driven) ---
    auto node = doc.append_child("polygon");
    node.append_attribute("points") = polygonCoords.c_str();

    Loops parser_loops;
    ASSERT_NO_THROW({ parser_loops = SvgParsers::parsePolygon(node, identity()); })
      << "Parser failed to process valid polygon string";

    auto result_parser = tess.process(parser_loops, GetParam().mode);
    const double area_parser = calculatePolygonArea(result_parser);

    // --- СРАВНЕНИЕ ---

    // 1. Проверка площади (самый важный показатель)
    // Используем допуск, так как при парсинге строк могут быть микро-ошибки float точности
    EXPECT_NEAR(area_manual, area_parser, 1e-4)
      << "Area mismatch! Parser might be losing precision or misinterpreting points.";

    // 2. Проверка количества вершин (количество треугольников должно совпасть)
    // Если тесселятор использует разные алгоритмы для разных типов ввода - это ошибка
    EXPECT_EQ(result_manual.size(), result_parser.size())
      << "Vertex count mismatch! Tessellator produces different triangulation for same shape.";

    // Проверка, что мы вообще получили треугольники
    EXPECT_GT(result_parser.size(), 0);
}

TEST_P(TesselationIntegrationTest, ParserWithTransform)
{
    auto group = doc.append_child("g");
    group.append_attribute("transform") = "translate(50, 50) scale(2, 1)"; // X*2 + 50, Y+50

    auto node = group.append_child("polygon");
    node.append_attribute("points") = "0,0 10,0 10,10";

    // Получаем трансформ группы (имитируем обход в SvgProcessor)
    auto parentTrans = identity();
    updateTransform(group, parentTrans);

    auto loops = SvgParsers::parsePolygon(node, parentTrans);
    Tesselate tess;
    auto result = tess.process(loops, GetParam().mode);

    // Точка (10,10) * Scale(2,1) + Trans(50,50) = 10 * 2 + 50; 10*1 + 50 = 70;60
    // Проверим последнюю точку треугольника/полигона
    bool foundCorrectPoint = false;
    for (const auto &p : result)
    { // Если это один контур
        if (std::abs(p.x() - 70.0) < 1e-3 && std::abs(p.y() - 60.0) < 1e-3)
        {
            foundCorrectPoint = true;
        }
    }
    // Здесь можно добавить более точные проверки на все вершины в зависимости от порядка
    EXPECT_TRUE(foundCorrectPoint);
    ASSERT_FALSE(result.empty());
}

// --- TEST FOR TESSELLATOR (The real check) ---

TEST_P(TesselationIntegrationTest, OpenVsClosedAreaTest)
{
    Tesselate tess;

    // 1. Замкнутый полигон
    auto poly_node = doc.append_child("polygon");
    poly_node.append_attribute("points") = "0,0 10,0 10,10";
    const auto poly_loops = SvgParsers::parsePolygon(poly_node, GlVertex::getIdentity());
    const auto res_poly = tess.process(poly_loops, GetParam().mode);

    const double area_poly = calculatePolygonArea(res_poly);

    // 2. Открытая полилиния
    auto line_node = doc.append_child("polyline");
    line_node.append_attribute("points") = "0,0 10,0 10,10";
    const auto polyline_loops = SvgParsers::parsePolygon(line_node, GlVertex::getIdentity());
    const auto res_polyLine = tess.process(polyline_loops, GetParam().mode);

    const double area_line = calculatePolygonArea(res_polyLine);

    // Проверяем: полигон имеет площадь (50), а линия — нет (0), но тесселятор все равно должен
    // замнкуть.
    EXPECT_NEAR(area_poly, 50.0, 1e-4);
    EXPECT_NEAR(area_line, area_poly, 1e-4);
}

// NOLINTNEXTLINE
INSTANTIATE_TEST_SUITE_P(TesselationAllModes, TesselationIntegrationTest,
                         ::testing::Values(TestParam{TessMode::Triangles, "Full Triangulation"},
                                           TestParam{TessMode::Contours, "Boundary Contours"}));
} // namespace Test
