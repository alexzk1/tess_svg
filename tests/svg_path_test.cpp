#include "pugixml.hpp"
#include "tess_svg/GlDefs.h"
#include "tess_svg/SvgPath.h"

#include <string>

#include <gtest/gtest.h>

namespace Test {

namespace {
class SvgPathTest : public SvgPath, public ::testing::Test
{
};
} // namespace

TEST_F(SvgPathTest, TransformParsing)
{
    // Translate
    {
        GlVertex::trans_matrix_t m = GlVertex::getIdentity();
        const std::string t = "translate(10, 20)";
        // Эмулируем работу SvgPath::transforms
        for (auto &f : SvgPathTest::transforms)
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
        for (auto &f : SvgPathTest::transforms)
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
        for (auto &f : SvgPathTest::transforms)
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

TEST_F(SvgPathTest, FullNodeInterfaceTest)
{
    pugi::xml_document doc;
    // Создаем структуру: <g transform="translate(10,20)"><path d="M 0 0 L 10 10" /></g>
    auto group = doc.append_child("g");
    group.append_attribute("transform") = "translate(100, 200) scale(2, 2)";

    auto path_node = group.append_child("path");
    path_node.append_attribute("d") = "M 0 0 L 10 10";

    // 1. Создаем объект (он вызовет parse_node внутри конструктора)
    const SvgPath path_obj(path_node, group);
    const auto &loops = path_obj.getLoops();

    ASSERT_FALSE(loops.empty());
    EXPECT_DOUBLE_EQ(loops[0][0].x(), 100.0);
    EXPECT_DOUBLE_EQ(loops[0][0].y(), 200.0);
}

TEST_F(SvgPathTest, CombinedInterfaceScaleTest)
{
    pugi::xml_document doc;
    auto group = doc.append_child("g");
    // Если код правильный, масштаб должен примениться ПОСЛЕ трансляции
    group.append_attribute("transform") = "translate(100, 200) scale(2, 2)";

    auto path_node = group.append_child("path");
    path_node.append_attribute("d") = "M 0 0 L 10 10";

    const SvgPath path_obj(path_node, group);
    const auto &loops = path_obj.getLoops();

    ASSERT_EQ(loops[0].size(), 2);

    // Вторая точка L 10 10.
    // Если scale(2,2) игнорируется, будет 110.
    // Если scale работает, будет 120.
    EXPECT_DOUBLE_EQ(loops[0][1].x(), 120.0);
}

TEST_F(SvgPathTest, NestedTransformTest)
{
    pugi::xml_document doc;
    // Родитель со сдвигом
    auto group = doc.append_child("g");
    group.append_attribute("transform") = "translate(10, 10)";

    // Путь со своим масштабом
    auto path_node = group.append_child("path");
    path_node.append_attribute("transform") = "scale(2, 2)";
    path_node.append_attribute("d") = "M 5 5";

    const SvgPath path_obj(path_node, group);
    const auto &loops = path_obj.getLoops();

    // ОЖИДАНИЕ:
    // Сначала Translate(10), потом Scale(2) для точки (5,5)
    // x = 10 + (5 * 2) = 20
    EXPECT_DOUBLE_EQ(loops[0][0].x(), 20.0);
}

TEST_F(SvgPathTest, CompactTransformParsing)
{
    pugi::xml_document doc;
    auto node = doc.append_child("path");
    // Без пробелов между командами
    node.append_attribute("transform") = "translate(10,10)scale(2,2)";
    node.append_attribute("d") = "M 0 0";

    // Используем пустую ноду как родителя
    const SvgPath path_obj(node, pugi::xml_node());
    const auto &loops = path_obj.getLoops();

    // Если регулярка в exec и поиск find(mask) работают верно,
    // точка (0,0) должна просто сдвинуться на 10.
    EXPECT_DOUBLE_EQ(loops[0][0].x(), 10.0);
}

TEST_F(SvgPathTest, MirroringScaleTest)
{
    pugi::xml_document doc;
    auto node = doc.append_child("path");
    node.append_attribute("transform") = "scale(-1, 1)"; // Отражение по X
    node.append_attribute("d") = "M 10 0";

    const SvgPath path_obj(node, pugi::xml_node());
    const auto &loops = path_obj.getLoops();

    // Точка 10 должна стать -10
    EXPECT_DOUBLE_EQ(loops[0][0].x(), -10.0);
}

TEST_F(SvgPathTest, RotationTest)
{
    pugi::xml_document doc;
    auto node = doc.append_child("path");
    node.append_attribute("transform") = "rotate(90)";
    node.append_attribute("d") = "M 10 0";

    const SvgPath path_obj(node, pugi::xml_node());
    const auto &loops = path_obj.getLoops();

    // После поворота на 90 градусов: x -> 0, y -> 10
    EXPECT_NEAR(loops[0][0].x(), 0.0, 1e-9);
    EXPECT_NEAR(loops[0][0].y(), 10.0, 1e-9);
}

TEST_F(SvgPathTest, DoubleSameTransformTest)
{
    pugi::xml_document doc;
    auto node = doc.append_child("path");
    // Две одинаковые команды подряд
    node.append_attribute("transform") = "translate(10, 10) translate(20, 20)";
    node.append_attribute("d") = "M 0 0";

    const SvgPath path_obj(node, pugi::xml_node());
    const auto &loops = path_obj.getLoops();

    // ОЖИДАНИЕ: 10 + 20 = 30
    EXPECT_DOUBLE_EQ(loops[0][0].x(), 30.0);
    EXPECT_DOUBLE_EQ(loops[0][0].y(), 30.0);
}

} // namespace Test
