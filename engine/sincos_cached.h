//
// Created by alex on 6/27/15.
//
#pragma once

#include <map>
#include <cmath>
#include <cstdint>
#include <limits>
#include "Vector2.h"

template <class T, int precission>
T roundTo(T value)
{
    if (std::numeric_limits<T>::is_integer)
        return value;
    constexpr T err = std::numeric_limits<T>::round_error() * std::numeric_limits<T>::epsilon();

    constexpr int64_t e = std::pow(10, precission);
    auto t = std::trunc(value  * e);

    T v = (T)t / (T)e + err;

    return (T)v;
}

template<class T, int precission>
typename std::enable_if < !std::numeric_limits<T>::is_integer, bool >::type almost_equal(T x, T y)
{
    return std::abs(x - y) <= std::numeric_limits<T>::epsilon();
}

template <class T>
class floats_comparer
{
public:
    bool operator()(T t1, T t2) const
    {
        return almost_equal<T>(t1, t2);
    }
};

template <typename T, int precission, size_t size_limit = 1024>
class sincos_cached
{
private:
    constexpr static int64_t e = std::pow(10, precission);
    std::map<uint64_t, T> sins;
    std::map<uint64_t, T> coss;
    static uint64_t getKey(T angle)
    {
        return std::abs(roundTo<T, precission>(angle) * e);
    }

public:

    T sin(T angle)
    {
        auto key = getKey(angle);

        T val;

        if (sins.count(key) > 0)
            val = sins.at(key); //using at to get exception - extra check
        else
        {
            val = std::abs(std::sin(angle));
            sins[key] = val;
        }

        while (sins.size() > size_limit)
            sins.erase(sins.begin());

        return val * ((angle < 0) ? -1 : 1);
    }

    T cos(T angle)
    {
        auto key = getKey(angle); //косинус четный, - можно не записывать
        T val;

        if (coss.count(key) > 0)
            val = coss.at(key);
        else
        {
            val = std::cos(angle);
            coss[key] = val;
        }

        while (coss.size() > size_limit)
            coss.erase(coss.begin());

        return val;
    }
};
