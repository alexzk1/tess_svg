#pragma once
#include <math.h>

#include <random>
#include <type_traits>

namespace mymath {
namespace NSfloats_Helper {
template <class T>
struct is_float
{
    static_assert(std::is_floating_point<T>::value, "T must be floating point one.");
    constexpr static bool value{sizeof(T) == sizeof(float)};
};

template <class T>
struct is_double
{
    static_assert(std::is_floating_point<T>::value, "T must be floating point one.");
    constexpr static bool value{sizeof(T) == sizeof(double)};
};

template <class T>
struct is_longdouble
{
    static_assert(std::is_floating_point<T>::value, "T must be floating point one.");
    constexpr static bool value{sizeof(T) == sizeof(long double)};
};
} // namespace NSfloats_Helper

namespace minmaxHelperNS {
// why so long code? because it is compile time condition (not runtime).
template <class T>
T min(const T &x, const T &y, std::true_type)
{
    static_assert(std::is_floating_point<T>::value, "T must be floating point one.");
    return std::fmin(x, y);
}

template <class T>
T min(const T &x, const T &y, std::false_type)
{
    static_assert(std::numeric_limits<T>::is_integer, "T must be integer type.");
    return std::min(x, y);
}

template <class T>
T max(const T &x, const T &y, std::true_type)
{
    static_assert(std::is_floating_point<T>::value, "T must be floating point one.");
    return std::fmax(x, y);
}

template <class T>
T max(const T &x, const T &y, std::false_type)
{
    static_assert(std::numeric_limits<T>::is_integer, "T must be integer type.");
    return std::max(x, y);
}
} // namespace minmaxHelperNS

template <class T>
T proper_min(const T &x, const T &y)
{
    return minmaxHelperNS::min<T>(x, y, std::is_floating_point<T>());
}

template <class T>
T proper_max(const T &x, const T &y)
{
    return minmaxHelperNS::max<T>(x, y, std::is_floating_point<T>());
}

template <class T>
inline T min(const T &x, const T &y)
{
    return proper_min<T>(x, y);
}

template <class T>
inline T min(const T &x, const T &y, const T &z)
{
    return proper_min<T>(proper_min<T>(x, y), z);
}

template <class T>
inline T min(const T &x, const T &y, const T &z, const T &w)
{
    return proper_min<T>(proper_min<T>(x, y), proper_min<T>(z, w));
}

template <class T>
inline T max(const T &x, const T &y)
{
    return proper_max<T>(x, y);
}

template <class T>
inline T max(const T &x, const T &y, const T &z)
{
    return proper_max<T>(proper_max<T>(x, y), z);
}

template <class T>
inline T max(const T &x, const T &y, const T &z, const T &w)
{
    return proper_max<T>(proper_max<T>(x, y), proper_max<T>(z, w));
}

namespace absHelperNS {
template <class T>
T abs(T val, std::true_type)
{
    static_assert(std::is_floating_point<T>::value, "T must be floating point one.");
    return std::fabs(val);
}
template <class T>
T abs(T val, std::false_type)
{
    static_assert(std::numeric_limits<T>::is_integer, "T must be integer type.");
    static_assert(std::is_signed<T>::value, "T must be signed or has no sence.");
    return std::abs(val);
}
} // namespace absHelperNS

template <class T>
inline T abs(T fValue)
{
    return absHelperNS::abs(fValue, std::is_floating_point<T>());
}

template <class T, class R = int>
inline R iCeil(T fValue)
{
    static_assert(std::numeric_limits<R>::is_integer, "R must be integer type.");
    static_assert(std::is_floating_point<T>::value, "T must be floating type.");
    return static_cast<R>(std::ceil(fValue));
}

template <class T>
inline bool isFinite(T x)
{
    static_assert(std::is_floating_point<T>::value, "T must be floating point one.");
    return std::isfinite(x);
}

template <class T>
inline T clamp(const T &v, const T &mn, const T &mx)
{
    return mymath::max<T>(mn, mymath::min<T>(v, mx));
}

template <typename T>
bool in_range(const T &lower, const T &upper, const T &val)
{
    static_assert(std::numeric_limits<T>::is_integer, "Integers only");
    return mymath::clamp<T>(val, lower, upper) == val;
}

namespace wrapHelperNS {
template <class T>
T iwrap(T val, T hi, std::true_type)
{
    static_assert(std::numeric_limits<T>::is_integer, "T must be integer type.");
    static_assert(std::is_signed<T>::value, "T must be signed one.");
    if (val < 0)
        return ((val % hi) + hi) % hi;
    else
        return val % hi;
}

template <class T>
T iwrap(T val, T hi, std::false_type)
{
    static_assert(std::numeric_limits<T>::is_integer, "T must be integer type.");
    static_assert(std::is_unsigned<T>::value, "T must be unsigned one.");
    return val % hi;
}
} // namespace wrapHelperNS

template <class T>
inline T iWrap(T val, T hi)
{
    return wrapHelperNS::iwrap<T>(val, hi, std::is_signed<T>::value);
}

template <class T, int precission>
inline T roundTo(T value)
{
    if (std::numeric_limits<T>::is_integer)
        return value;
    constexpr T err = std::numeric_limits<T>::round_error() * std::numeric_limits<T>::epsilon();

    constexpr int64_t e = std::pow(10, precission);
    auto t = std::trunc(value * e);

    T v = (T)t / (T)e + err;

    return (T)v;
}

template <class T>
inline bool almost_equal(T x, T y)
{
    return mymath::abs(x - y) <= std::numeric_limits<T>::epsilon();
}

template <class T>
class floats_comparer
{
  public:
    bool operator()(T t1, T t2) const
    {
        static_assert(std::is_floating_point<T>::value, "T must be floating point one.");
        return almost_equal<T>(t1, t2);
    }
};

// calculatuion of sin & cos in 1 call using GNU extension if possible
template <class T>
struct sincos_res
{
    static_assert(std::is_floating_point<T>::value, "T must be floating point one.");
    T sin;
    T cos;
};
#ifdef _GNU_SOURCE
namespace NS_GNUscPicker {
// that what happens when C does not have overloads....
template <class T>
auto sincos_ld(T x, std::false_type)
{
    static_assert(NSfloats_Helper::is_longdouble<T>::value, "Woops!");
    sincos_res<T> res;
    ::sincosl(x, &res.sin, &res.cos);
    return res;
}

template <class T>
auto sincos_ld(T x, std::true_type)
{
    static_assert(NSfloats_Helper::is_double<T>::value, "Woops!");
    sincos_res<T> res;
    ::sincos(x, &res.sin, &res.cos);
    return res;
}

template <class T>
auto sincos_fd(T x, std::false_type)
{
    // pick double or long double
    static_assert(!NSfloats_Helper::is_float<T>::value, "Woops!");
    return NS_GNUscPicker::sincos_ld<T>(
      x, std::integral_constant<bool, NSfloats_Helper::is_double<T>::value>{});
}

template <class T>
auto sincos_fd(T x, std::true_type)
{
    static_assert(NSfloats_Helper::is_float<T>::value, "Woops!");
    // pick float
    sincos_res<T> res;
    ::sincosf(x, &res.sin, &res.cos);
    return res;
}
} // namespace NS_GNUscPicker
template <class T>
inline auto sincos(T x)
{
    static_assert(std::is_floating_point<T>::value, "T must be floating point one.");
    return NS_GNUscPicker::sincos_fd<T>(
      x, std::integral_constant<bool, NSfloats_Helper::is_float<T>::value>{});
}
#else
template <class T>
inline auto sincos(T x)
{
    static_assert(std::is_floating_point<T>::value, "T must be floating point one.");
    sincos_res<T> res;
    res.sin = std::sin(x);
    res.cos = std::cos(x);
    return res;
}
#endif

} // namespace mymath

namespace myrnd {
template <class T = float>
inline T uniformRandom(T low = static_cast<T>(0.), T hi = static_cast<T>(1.))
{
    static_assert(std::is_floating_point<T>::value, "T must be floating point one.");
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<T> dis(low, hi);
    return dis(gen);
}

//----------------------------------------------------------------------------
template <class T = float>
inline T gaussRandom(T mean = 0.0f, T stdev = 1.0f)
{
    static_assert(std::is_floating_point<T>::value, "T must be floating point one.");
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<T> dis(mean, stdev);
    return dis(gen);
}

inline bool randomBool()
{
    return uniformRandom<float>(0.f, 1.f) < 0.5;
}
} // namespace myrnd
