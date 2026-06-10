#include "pugixml.hpp"
#include "tess_svg/GlDefs.h"
#include "tess_svg/SvgParsers.h"
#include "tess_svg/node_transform.hpp"

#include <cstdlib>
#include <string>

#include <gtest/gtest.h>

namespace Test {

namespace {
class SvgParsersTest : public ::testing::Test
{
  public:
    static GlVertex::trans_matrix_t getParentTransform(const pugi::xml_node &parent)
    {
        GlVertex::trans_matrix_t parentTrans = GlVertex::getIdentity();
        updateTransform(parent, parentTrans);
        return parentTrans;
    }

  protected:
    pugi::xml_document doc;
};
} // namespace

TEST_F(SvgParsersTest, TransformParsing)
{
    // Translate
    {
        GlVertex::trans_matrix_t m = GlVertex::getIdentity();
        const std::string t = "translate(10, 20)";
        // Эмулируем работу SvgPath::transforms
        for (auto &f : trans_detals::getTransformParsers())
        {
            if (f.exec(t, m))
            {
                break;
            }
        }
        EXPECT_DOUBLE_EQ(m(0, 2), 10.0);
        EXPECT_DOUBLE_EQ(m(1, 2), 20.0);
    }

    // Scale
    {
        GlVertex::trans_matrix_t m = GlVertex::getIdentity();
        const std::string t = "scale(2, 3)";
        for (auto &f : trans_detals::getTransformParsers())
        {
            if (f.exec(t, m))
            {
                break;
            }
        }
        EXPECT_DOUBLE_EQ(m(0, 0), 2.0);
        EXPECT_DOUBLE_EQ(m(1, 1), 3.0);
    }

    // Matrix
    {
        GlVertex::trans_matrix_t m = GlVertex::getIdentity();
        // matrix(a,b,c,d,e,f) -> a=1, b=0, c=0, d=1, e=50, f=60 (это просто translate)
        const std::string t = "matrix(1, 0, 0, 1, 50, 60)";
        for (auto &f : trans_detals::getTransformParsers())
        {
            if (f.exec(t, m))
            {
                break;
            }
        }
        EXPECT_DOUBLE_EQ(m(0, 2), 50.0);
        EXPECT_DOUBLE_EQ(m(1, 2), 60.0);
    }
}

TEST_F(SvgParsersTest, FullNodeInterfaceTest)
{
    // Создаем структуру: <g transform="translate(10,20)"><path d="M 0 0 L 10 10" /></g>
    auto group = doc.append_child("g");
    group.append_attribute("transform") = "translate(100, 200) scale(2, 2)";

    auto path_node = group.append_child("path");
    path_node.append_attribute("d") = "M 0 0 L 10 10";

    const auto loops = SvgParsers::parsePath(path_node, getParentTransform(group));

    ASSERT_FALSE(loops.empty());
    EXPECT_DOUBLE_EQ(loops[0][0].x(), 100.0);
    EXPECT_DOUBLE_EQ(loops[0][0].y(), 200.0);
}

TEST_F(SvgParsersTest, CombinedInterfaceScaleTest)
{
    auto group = doc.append_child("g");
    // Если код правильный, масштаб должен примениться ПОСЛЕ трансляции
    group.append_attribute("transform") = "translate(100, 200) scale(2, 2)";

    auto path_node = group.append_child("path");
    path_node.append_attribute("d") = "M 0 0 L 10 10";

    const auto loops = SvgParsers::parsePath(path_node, getParentTransform(group));

    ASSERT_EQ(loops[0].size(), 2);

    // Вторая точка L 10 10.
    // Если scale(2,2) игнорируется, будет 110.
    // Если scale работает, будет 120.
    EXPECT_DOUBLE_EQ(loops[0][1].x(), 120.0);
}

TEST_F(SvgParsersTest, NestedTransformTest)
{
    // Родитель со сдвигом
    auto group = doc.append_child("g");
    group.append_attribute("transform") = "translate(10, 10)";

    // Путь со своим масштабом
    auto path_node = group.append_child("path");
    path_node.append_attribute("transform") = "scale(2, 2)";
    path_node.append_attribute("d") = "M 5 5";

    const auto loops = SvgParsers::parsePath(path_node, getParentTransform(group));

    // ОЖИДАНИЕ:
    // Сначала Translate(10), потом Scale(2) для точки (5,5)
    // x = 10 + (5 * 2) = 20
    EXPECT_DOUBLE_EQ(loops[0][0].x(), 20.0);
}

TEST_F(SvgParsersTest, CompactTransformParsing)
{
    auto node = doc.append_child("path");
    // Без пробелов между командами
    node.append_attribute("transform") = "translate(10,10)scale(2,2)";
    node.append_attribute("d") = "M 0 0";

    const auto loops = SvgParsers::parsePath(node, getParentTransform({}));

    // Если регулярка в exec и поиск find(mask) работают верно,
    // точка (0,0) должна просто сдвинуться на 10.
    EXPECT_DOUBLE_EQ(loops[0][0].x(), 10.0);
}

TEST_F(SvgParsersTest, MirroringScaleTest)
{
    auto node = doc.append_child("path");
    node.append_attribute("transform") = "scale(-1, 1)"; // Отражение по X
    node.append_attribute("d") = "M 10 0";

    const auto loops = SvgParsers::parsePath(node, getParentTransform({}));

    // Точка 10 должна стать -10
    EXPECT_DOUBLE_EQ(loops[0][0].x(), -10.0);
}

TEST_F(SvgParsersTest, RotationTest)
{
    auto node = doc.append_child("path");
    node.append_attribute("transform") = "rotate(90)";
    node.append_attribute("d") = "M 10 0";

    const auto loops = SvgParsers::parsePath(node, getParentTransform({}));

    // После поворота на 90 градусов: x -> 0, y -> 10
    EXPECT_NEAR(loops[0][0].x(), 0.0, 1e-9);
    EXPECT_NEAR(loops[0][0].y(), 10.0, 1e-9);
}

TEST_F(SvgParsersTest, DoubleSameTransformTest)
{
    auto node = doc.append_child("path");
    // Две одинаковые команды подряд
    node.append_attribute("transform") = "translate(10, 10) translate(20, 20)";
    node.append_attribute("d") = "M 0 0";

    const auto loops = SvgParsers::parsePath(node, getParentTransform({}));

    // ОЖИДАНИЕ: 10 + 20 = 30
    EXPECT_DOUBLE_EQ(loops[0][0].x(), 30.0);
    EXPECT_DOUBLE_EQ(loops[0][0].y(), 30.0);
}

// --- RECT ---
TEST_F(SvgParsersTest, RectBasic)
{
    auto node = doc.append_child("rect");
    node.append_attribute("x") = "10";
    node.append_attribute("y") = "20";
    node.append_attribute("width") = "30";
    node.append_attribute("height") = "40";

    auto loops = SvgParsers::parseRect(node, GlVertex::getIdentity());
    ASSERT_EQ(loops.size(), 1);
    EXPECT_NEAR(loops[0][0].x(), 10.0, 1e-5); // (x)
    EXPECT_NEAR(loops[0][2].x(), 40.0, 1e-5); // (x + w)
}

TEST_F(SvgParsersTest, RectTransformed)
{
    auto group = doc.append_child("g");
    group.append_attribute("transform") = "translate(100, 100)";
    auto node = group.append_child("rect");
    node.append_attribute("x") = "5";
    node.append_attribute("y") = "5";
    node.append_attribute("width") = "10";
    node.append_attribute("height") = "10";

    auto loops = SvgParsers::parseRect(node, getParentTransform(group));
    // (5+100) = 105
    EXPECT_NEAR(loops[0][0].x(), 105.0, 1e-5);
}

// --- CIRCLE & ELLIPSE ---
TEST_F(SvgParsersTest, EllipseBasic)
{
    auto node = doc.append_child("ellipse");
    node.append_attribute("cx") = "0";
    node.append_attribute("cy") = "0";
    node.append_attribute("rx") = "20";
    node.append_attribute("ry") = "10";

    auto loops = SvgParsers::parseEllipse(node, GlVertex::getIdentity());
    ASSERT_EQ(loops.size(), 1);
    // Проверка точки на оси X: (cx + rx) = 20
    bool foundRightPoint = false;
    for (const auto &p : loops[0])
    {
        if (std::abs(p.x() - 20.0) < 1e-3 && std::abs(p.y()) < 1e-3)
        {
            foundRightPoint = true;
        }
    }
    EXPECT_TRUE(foundRightPoint);
}

TEST_F(SvgParsersTest, CircleTransformScale)
{
    auto group = doc.append_child("g");
    group.append_attribute("transform") = "scale(2, 3)";
    auto node = group.append_child("circle");
    node.append_attribute("cx") = "10";
    node.append_attribute("cy") = "10";
    node.append_attribute("r") = "5";

    auto loops = SvgParsers::parseCircle(node, getParentTransform(group));
    // Центр после scale: (20, 30). Точка справа (cx+r): (25, 30) -> scale -> (50, 90)?
    // Нет! Масштаб применяется к КООРДИНАТАМ точек.
    // Локально точка на правом краю: x=15, y=10. После scale(2,3): x=30, y=30.
    bool foundPoint = false;
    for (const auto &p : loops[0])
    {
        if (std::abs(p.x() - 30.0) < 1e-3 && std::abs(p.y() - 30.0) < 1e-3)
        {
            foundPoint = true;
        }
    }
    EXPECT_TRUE(foundPoint);
}

// --- LINE ---
TEST_F(SvgParsersTest, LineBasic)
{
    auto node = doc.append_child("line");
    node.append_attribute("x1") = "0";
    node.append_attribute("y1") = "0";
    node.append_attribute("x2") = "100";
    node.append_attribute("y2") = "50";

    auto loops = SvgParsers::parseLine(node, GlVertex::getIdentity());
    ASSERT_EQ(loops.size(), 1);
    EXPECT_NEAR(loops[0][0].x(), 0.0, 1e-5);
    EXPECT_NEAR(loops[0][1].x(), 100.0, 1e-5);
}

TEST_F(SvgParsersTest, PolygonTransformed)
{
    auto group = doc.append_child("g");
    group.append_attribute("transform") = "translate(50, 50)";
    auto node = group.append_child("polygon");
    node.append_attribute("points") = "0,0 10,0 10,10";

    auto loops = SvgParsers::parsePolygon(node, getParentTransform(group));
    // Точка (10,10) после translate(50,50) должна стать (60,60)
    EXPECT_NEAR(loops[0][2].x(), 60.0, 1e-5);
    EXPECT_NEAR(loops[0][2].y(), 60.0, 1e-5);
}

// --- POLYGON TESTS ---

TEST_F(SvgParsersTest, PolygonIsClosed)
{
    auto node = doc.append_child("polygon");
    node.append_attribute("points") = "0,0 10,0 10,10"; // Три точки (треугольник)

    // В SVG polygon должен автоматически замкнуться от (10,10) к (0,0)
    auto loops = SvgParsers::parsePolygon(node, GlVertex::getIdentity());

    ASSERT_EQ(loops.size(), 1);
    ASSERT_EQ(loops[0].size(), 4) << "Polygon should have an implicit closed point!";
    EXPECT_NEAR(loops[0][3].x(), 0.0, 1e-5); // Четвертая точка должна совпадать с первой
}

TEST_F(SvgParsersTest, PolylineIsOpen)
{
    auto node = doc.append_child("polyline");
    node.append_attribute("points") = "0,0 10,0 10,10"; // Три точки (просто ломаная линия)

    auto loops = SvgParsers::parsePolygon(node, GlVertex::getIdentity());

    ASSERT_EQ(loops.size(), 1);
    EXPECT_EQ(loops[0].size(), 3) << "Polyline should NOT be closed automatically";
}

} // namespace Test
