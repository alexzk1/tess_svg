#include "tess_svg/GlDefs.h"
#include "tess_svg/tagdparser.h"

#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <string>

#include <gtest/gtest.h>

extern float BEIZER_FLATNESS;
extern std::size_t ELLIPSE_POINTS;

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
    // c 1 1 2 2 3 3 (заканчивается в 3,3)
    // потом еще 4 4 5 5 6 6 (относительно 3,3 это будет 9,9)
    const std::string path_data = "m 0 0 c 1 1 2 2 3 3 4 4 5 5 6 6";
    auto loops = TagDParser::split(path_data, createIdentityMatrix());

    ASSERT_FALSE(loops.empty());

    // Правильный результат накопления относительных координат
    EXPECT_NEAR(loops[0].back().x(), 9.0, 0.001);
    EXPECT_NEAR(loops[0].back().y(), 9.0, 0.001);

    // Поскольку это прямая линия, адаптивный алгоритм даст мало точек
    // (всего по 2 на сегмент + стартовая = 5 точек)
    EXPECT_LT(loops[0].size(), 10);
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

// Тест на эллиптическую арку (команда A/a)
TEST(TagDParserTest, EllipticArcCommand)
{
    // M 0 0 - начало
    // A 10 20 0 0 1 10 10
    // rx=10, ry=20, rot=0, large=0, sweep=1, end=(10,10)
    const std::string path_data = "M 0 0 A 10 20 0 0 1 10 10";
    const auto loops = TagDParser::split(path_data, createIdentityMatrix());

    ASSERT_FALSE(loops.empty());
    // Проверяем, что арка дошла до целевой точки
    EXPECT_NEAR(loops[0].back().x(), 10.0, 0.001);
    EXPECT_NEAR(loops[0].back().y(), 10.0, 0.001);

    // Проверяем количество точек тесселяции (зависит от ELLIPSE_POINTS)
    // Для четверти эллипса при 1024 точках на полный круг должно быть ~256 точек
    EXPECT_GT(loops[0].size(), 100);
}

// Тест на вырожденную арку (нулевой радиус должен стать линией)
TEST(TagDParserTest, DegenerateArcIsLine)
{
    const std::string path_data = "M 0 0 A 0 0 0 0 1 50 50";
    const auto loops = TagDParser::split(path_data, createIdentityMatrix());

    ASSERT_EQ(loops[0].size(), 2); // Только Start и End
    EXPECT_DOUBLE_EQ(loops[0].back().x(), 50.0);
}

// Тест на коррекцию радиусов (если rx/ry слишком малы, чтобы достать до точки)
TEST(TagDParserTest, ArcRadiusCorrection)
{
    // Точки на расстоянии 100, а радиус всего 1.
    // Алгоритм должен пропорционально увеличить rx/ry до 50+.
    const std::string path_data = "M 0 0 a 1 1 0 0 1 100 0";
    EXPECT_NO_THROW({
        const auto loops = TagDParser::split(path_data, createIdentityMatrix());
        EXPECT_NEAR(loops[0].back().x(), 100.0, 0.001);
    });
}

TEST(TagDParserTest, LowPrecisionArc)
{
    auto original_points = ELLIPSE_POINTS;

    // Делаем эллипс очень грубым (всего 4 точки на круг)
    ELLIPSE_POINTS = 4;

    const std::string path_data = "M 0 0 A 10 10 0 0 1 20 0"; // полуокружность
    auto loops = TagDParser::split(path_data, createIdentityMatrix());

    // Полуокружность (180 градусов) при 4 точках на 360 градусов
    // должна дать примерно 2 сегмента (3 точки: старт, середина, конец)
    EXPECT_LT(loops[0].size(), 5);

    ELLIPSE_POINTS = original_points; // Возвращаем назад!
}

// Тест на адаптивность квадратичной кривой Безье (Q/q)
TEST(TagDParserTest, AdaptiveQuadraticBezier)
{
    // Сохраняем старое значение, чтобы не сломать другие тесты
    const float original_flatness = BEIZER_FLATNESS;

    const std::string path_data = "M 0 0 Q 50 100 100 0";

    // 1. Проверка при грубой настройке (высокий допуск)
    BEIZER_FLATNESS = 10.0f;
    auto loops_rough = TagDParser::split(path_data, createIdentityMatrix());
    const std::size_t points_rough = loops_rough[0].size();

    // 2. Проверка при точной настройке (низкий допуск)
    BEIZER_FLATNESS = 0.1f;
    auto loops_precise = TagDParser::split(path_data, createIdentityMatrix());
    const std::size_t points_precise = loops_precise[0].size();

    // Точек при 0.1 должно быть значительно больше, чем при 10.0
    EXPECT_GT(points_precise, points_rough);

    // Проверяем, что кривая всё равно пришла в конечную точку
    EXPECT_NEAR(loops_precise[0].back().x(), 100.0, 0.001);
    EXPECT_NEAR(loops_precise[0].back().y(), 0.0, 0.001);

    BEIZER_FLATNESS = original_flatness; // Восстанавливаем
}

// Тест на адаптивность кубической кривой Безье (C/c)
TEST(TagDParserTest, AdaptiveCubicBezier)
{
    const float original_flatness = BEIZER_FLATNESS;

    // Сложная S-образная кривая
    const std::string path_data = "M 0 0 C 0 100 100 100 100 0";

    // 1. Высокий допуск (мало точек)
    BEIZER_FLATNESS = 5.0f;
    auto loops_rough = TagDParser::split(path_data, createIdentityMatrix());

    // 2. Очень низкий допуск (много точек)
    BEIZER_FLATNESS = 0.05f;
    auto loops_precise = TagDParser::split(path_data, createIdentityMatrix());

    EXPECT_GT(loops_precise[0].size(), loops_rough[0].size());

    // Проверка целостности: первая и последняя точки должны быть на месте
    EXPECT_NEAR(loops_precise[0].front().x(), 0.0, 0.001);
    EXPECT_NEAR(loops_precise[0].back().x(), 100.0, 0.001);

    BEIZER_FLATNESS = original_flatness;
}

// Тест на "вырожденную" кривую (когда точки лежат на одной прямой)
TEST(TagDParserTest, DegenerateBezierFlatness)
{
    const float original_flatness = BEIZER_FLATNESS;
    BEIZER_FLATNESS = 0.5f;

    // Контрольные точки лежат прямо на линии между стартом и концом
    // M 0,0 -> C 33,0 66,0 -> 100,0
    const std::string path_data = "M 0 0 C 33 0 66 0 100 0";
    const auto loops = TagDParser::split(path_data, createIdentityMatrix());

    // Для прямой линии рекурсия должна сработать мгновенно и дать минимум точек
    // (обычно это старт и конец, или пара делений)
    EXPECT_LT(loops[0].size(), 5);

    BEIZER_FLATNESS = original_flatness;
}

// Тест на адаптивность квадратичной кривой Безье (Q/q)
TEST(TagDParserTest, AdaptiveQuadraticBezier2)
{
    // 1. Сохраняем исходное значение, чтобы восстановить в конце
    const float original_flatness = BEIZER_FLATNESS;

    // Парабола: из (0,0) в (100,0) через вершину (50,100)
    const std::string path_data = "M 0 0 Q 50 100 100 0";

    // 2. Тестируем "Грубый" режим (Flatness = 10 пикселей)
    // Алгоритм должен допустить большую погрешность и дать мало точек
    BEIZER_FLATNESS = 10.0f;
    auto loops_rough = TagDParser::split(path_data, createIdentityMatrix());
    const std::size_t points_rough = loops_rough[0].size();

    // 3. Тестируем "Точный" режим (Flatness = 0.1 пикселя)
    // Алгоритм должен дробить сегменты, пока кривая не станет идеально гладкой
    BEIZER_FLATNESS = 0.1f;
    auto loops_precise = TagDParser::split(path_data, createIdentityMatrix());
    const std::size_t points_precise = loops_precise[0].size();

    // --- ПРОВЕРКИ ---

    // Точек в точном режиме должно быть значительно больше
    EXPECT_GT(points_precise, points_rough)
      << "Precise mode should generate more points than rough mode";

    // Проверяем, что кривая всё равно заканчивается там, где должна
    ASSERT_FALSE(loops_precise[0].empty());
    EXPECT_NEAR(loops_precise[0].back().x(), 100.0, 0.001);
    EXPECT_NEAR(loops_precise[0].back().y(), 0.0, 0.001);

    // Проверяем симметрию (для параболы Q 50 100 средняя точка должна быть около x=50)
    bool found_middle = false;
    for (const auto &v : loops_precise[0])
    {
        if (std::abs(v.x() - 50.0) < 1.0)
        {
            found_middle = true;
            break;
        }
    }
    EXPECT_TRUE(found_middle) << "Bezier curve should pass through the middle area around X=50";

    // 4. Восстанавливаем глобальное состояние
    BEIZER_FLATNESS = original_flatness;
}

TEST(TagDParserTest, BezierPointsOrder)
{
    const float original_flatness = BEIZER_FLATNESS;
    BEIZER_FLATNESS = 1.0f; // Достаточная точность

    // Тестируем кубическую кривую C
    const std::string path_data = "M 0 0 C 50 100 150 -100 200 0";
    const auto loops = TagDParser::split(path_data, createIdentityMatrix());

    ASSERT_FALSE(loops.empty());
    const auto &points = loops[0];
    ASSERT_GT(points.size(), 2);

    // Проверяем, что первая точка - старт, последняя - конец
    EXPECT_NEAR(points.front().x(), 0.0, 0.001);
    EXPECT_NEAR(points.back().x(), 200.0, 0.001);

    // Главная проверка: каждая следующая точка должна быть "дальше" по параметру t.
    // Для простой S-кривой вдоль оси X мы можем проверить, что X растет (или хотя бы не прыгает
    // хаотично) Но лучше проверить, что сумма расстояний между соседними точками адекватна.

    float total_path_length = 0;
    for (size_t i = 1; i < points.size(); ++i)
    {
        const auto dx = points[i].x() - points[i - 1].x();
        const auto dy = points[i].y() - points[i - 1].y();
        const auto dist = std::sqrt(dx * dx + dy * dy);

        // Если точки перепутаны, у нас будут огромные скачки назад-вперед
        // Расстояние между соседними точками в адаптивном делении не должно
        // быть больше, чем, скажем, 1/2 всей длины кривой (очень грубый фильтр)
        EXPECT_LT(dist, 150.0) << "Points are likely out of order at index " << i;
        total_path_length += dist;
    }

    // Для кривой от 0 до 200 общая длина пути должна быть чуть больше 200,
    // но если точки прыгают туда-сюда (0 -> 200 -> 100), длина будет огромной.
    EXPECT_LT(total_path_length, 600.0) << "Total path length is too high, points are jumping!";

    const float direct_dist = std::sqrt(std::pow(points.back().x() - points.front().x(), 2)
                                        + std::pow(points.back().y() - points.front().y(), 2));

    // Сумма длин всех сегментов (total_path_length) не должна быть
    // в 10 раз больше, чем расстояние по прямой (если только это не бешеная спираль)
    EXPECT_LT(total_path_length, direct_dist * 5.0)
      << "Path is too 'noisy' or points were swapped, total_path_length: " << total_path_length;

    BEIZER_FLATNESS = original_flatness;
}

} // namespace Test
