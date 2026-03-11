#include "tess_svg/GlDefs.h"
#include "tess_svg/tagdparser.h"

#include <string>

#include <gtest/gtest.h>

namespace Test {

namespace {
auto createIdentityMatrix()
{
    return GlVertex::getIdentity();
}
} // namespace

TEST(TagDParserTest, ImplicitLineCommands)
{
    // M 0 0, а потом сразу пачка координат для L
    const std::string path_data = "M 0 0 10 10 20 0 30 10";
    const auto loops = TagDParser::split(path_data, createIdentityMatrix());

    ASSERT_EQ(loops.size(), 1);
    ASSERT_EQ(loops[0].size(), 4); // M + 3 точки L

    EXPECT_DOUBLE_EQ(loops[0][3].x(), 30.0);
    EXPECT_DOUBLE_EQ(loops[0][3].y(), 10.0);
}

TEST(TagDParserTest, ImplicitCubicBezier)
{
    // Одна команда 'c' на два сегмента (12 чисел)
    const std::string path_data = "m 0 0 c 1 1 2 2 3 3 4 4 5 5 6 6";
    auto loops = TagDParser::split(path_data, createIdentityMatrix());

    // Каждый сегмент Безье при BEIZER_PARTS=10 дает 11 точек
    // У нас 2 сегмента. Точек должно быть много.
    ASSERT_FALSE(loops.empty());
    // Проверяем финальную точку последнего сегмента
    EXPECT_NEAR(loops[0].back().x(), 6.0, 0.001);
}

// Тест на слипшиеся координаты с минусами
TEST(TagDParserTest, StickyCoordinatesWithNegatives)
{
    const std::string path_data = "M10-20L-30.5-40.7"; // Типичный SVG "из ада"
    const auto loops = TagDParser::split(path_data, createIdentityMatrix());
    ASSERT_EQ(loops[0].size(), 2);
    EXPECT_DOUBLE_EQ(loops[0][0].x(), 10.0);
    EXPECT_DOUBLE_EQ(loops[0][0].y(), -20.0);
    EXPECT_DOUBLE_EQ(loops[0][1].x(), -30.5);
}

// Тест на сброс зеркала (преемственность команд)
TEST(TagDParserTest, MirrorResetLogic)
{
    // Сначала рисуем кривую (задается зеркало), потом линию (сбрасывает зеркало),
    // потом S (должна взять default зеркало)
    const std::string path_data = "M 0 0 C 10 10 20 10 30 0 L 40 0 S 50 10 60 0";

    auto loops = TagDParser::split(path_data, createIdentityMatrix());
    // Если зеркало сбросилось правильно, S отработает как обычная кривая Bezier
    // от точки (40,0) с контрольной точкой, совпадающей с (40,0)
    EXPECT_NO_THROW(TagDParser::split(path_data, createIdentityMatrix()));
    EXPECT_NEAR(loops[0].back().x(), 60.0, 0.001);
}

TEST(TagDParserTest, RelativeCoordinatesAccumulation)
{
    // m (10,10) -> точка 10,10
    // l (10,10) -> 10+10, 10+10 = 20,20
    // l (-5, 5) -> 20-5, 20+5   = 15,25
    const std::string path_data = "m 10 10 l 10 10 l -5 5";
    const auto loops = TagDParser::split(path_data, createIdentityMatrix());

    ASSERT_EQ(loops.size(), 1);
    ASSERT_EQ(loops[0].size(), 3);

    EXPECT_DOUBLE_EQ(loops[0][2].x(), 15.0);
    EXPECT_DOUBLE_EQ(loops[0][2].y(), 25.0);
}

TEST(TagDParserTest, ClosePathCommand)
{
    const std::string path_data = "M 0 0 L 10 0 L 10 10 Z";
    const auto loops = TagDParser::split(path_data, createIdentityMatrix());

    ASSERT_EQ(loops.size(), 1);
    // В зависимости от твоей реализации, Z может либо добавлять точку начала,
    // либо просто закрывать текущий loop.
    // Проверь, что последняя точка совпадает с первой (0,0)
    EXPECT_DOUBLE_EQ(loops[0].back().x(), 0.0);
    EXPECT_DOUBLE_EQ(loops[0].back().y(), 0.0);
}

TEST(TagDParserTest, WhitespaceAndCommas)
{
    // Запятые между координатами и лишние пробелы
    const std::string path_data = "M,0,0  L 10,10   , 20 20";
    const auto loops = TagDParser::split(path_data, createIdentityMatrix());

    ASSERT_EQ(loops.size(), 1);
    ASSERT_EQ(loops[0].size(), 3); // M + 2 точки L
    EXPECT_DOUBLE_EQ(loops[0][2].x(), 20.0);
}

TEST(TagDParserTest, ComplexBezierSequence)
{
    // M 0,0
    // C 10,10 20,10 30,0  (контрольные точки 10,10 и 20,10)
    // S 50,10 60,0        (должна зеркалить 20,10 относительно 30,0 -> x1,y1 будет 40,-10)
    const std::string path_data = "M 0 0 C 10 10 20 10 30 0 S 50 10 60 0";
    const auto loops = TagDParser::split(path_data, createIdentityMatrix());

    ASSERT_FALSE(loops.empty());
    EXPECT_NEAR(loops[0].back().x(), 60.0, 0.001);
}

TEST(TagDParserTest, PrickleTestImplicitStartAndMAsL)
{
    // 1. Начинаем сразу с относительной линии 'l' (без M)
    // По спецификации: начальная точка 0,0. 'l 10 10' должно привести в (10,10)
    // 2. Затем используем 'm 10 10 10 0'
    // По спецификации: первая пара - это перемещение (в 20,20),
    // а вторая пара (10, 0) - это уже ЛИНИЯ относительно (20,20) -> (30,20)

    const std::string path_data = "l 10 10 m 10 10 10 0";
    const auto loops = TagDParser::split(path_data, createIdentityMatrix());

    // Если всё ок, у нас должен быть один контур (или два, если m разрывает его)
    // В твоей реализации m обычно очищает current_path, так что будет 2 лупа.
    ASSERT_GE(loops.size(), 1);

    // Проверим финальную точку последнего лупа
    const auto &last_loop = loops.back();
    ASSERT_FALSE(last_loop.empty());

    // (10,10) + (10,10) [move] + (10,0) [implicit line] = (30, 20)
    EXPECT_NEAR(last_loop.back().x(), 30.0, 0.001);
    EXPECT_NEAR(last_loop.back().y(), 20.0, 0.001);
}

TEST(TagDParserTest, HorizontalAndVerticalLines)
{
    // M 10 10 -> точка (10,10)
    // h 10    -> (20,10)  относительно
    // V 40    -> (20,40)  абсолютно по вертикали
    // v -10   -> (20,30)  относительно по вертикали
    // H 0     -> (0,30)   абсолютно по горизонтали
    const std::string path_data = "M 10 10 h 10 V 40 v -10 H 0";
    const auto loops = TagDParser::split(path_data, createIdentityMatrix());

    ASSERT_EQ(loops[0].size(), 5);
    EXPECT_DOUBLE_EQ(loops[0][1].x(), 20.0); // после h 10
    EXPECT_DOUBLE_EQ(loops[0][2].y(), 40.0); // после V 40
    EXPECT_DOUBLE_EQ(loops[0][3].y(), 30.0); // после v -10
    EXPECT_DOUBLE_EQ(loops[0][4].x(), 0.0);  // после H 0
}

TEST(TagDParserTest, MultipleSubpaths)
{
    // Два отдельных треугольника в одной строке
    const std::string path_data = "M 0 0 L 10 0 L 0 10 Z M 50 50 L 60 50 L 50 60 Z";
    const auto loops = TagDParser::split(path_data, createIdentityMatrix());

    ASSERT_EQ(loops.size(), 2); // Должно быть два отдельных контура
    EXPECT_DOUBLE_EQ(loops[0][0].x(), 0.0);
    EXPECT_DOUBLE_EQ(loops[1][0].x(), 50.0);
}

TEST(TagDParserTest, RelativeMoveAfterClosePath)
{
    // 1. Начинаем в (10,10)
    // 2. Рисуем линию в (20,10)
    // 3. Закрываем (возвращаемся в 10,10)
    // 4. m 10 10 -> должно переместить нас в (20,20), так как база теперь (10,10)
    const std::string path_data = "M 10 10 L 20 10 Z m 10 10 L 30 20";
    const auto loops = TagDParser::split(path_data, createIdentityMatrix());

    ASSERT_EQ(loops.size(), 2);
    // Начало второго контура должно быть (20, 20)
    EXPECT_DOUBLE_EQ(loops[1][0].x(), 20.0);
    EXPECT_DOUBLE_EQ(loops[1][0].y(), 20.0);
}

TEST(TagDParserTest, FractionalNumbersWithoutLeadingZero)
{
    const std::string path_data = "M .5 .5 L -.5 1.5";
    const auto loops = TagDParser::split(path_data, createIdentityMatrix());

    ASSERT_EQ(loops[0].size(), 2);
    EXPECT_DOUBLE_EQ(loops[0][0].x(), 0.5);
    EXPECT_DOUBLE_EQ(loops[0][1].x(), -0.5);
}

} // namespace Test
