//
// Created by alex on 6/27/15.
//
#pragma once
#include <map>
#include <cmath>
#include <cstdint>
#include <limits>
#include "Vector2.h"
#include "my_math.h"

template <typename T, int precission, size_t size_limit = 1024>
class sincos_cached
{
private:
    constexpr static int64_t e = std::pow(10, precission);
    std::map<uint64_t, mymath::sincos_res<T>> cache;

    uint64_t getKey(T angle) const
    {
        return mymath::abs(mymath::roundTo<T, precission>(angle) * e);
    }

    void calcAndCache(T angle, uint64_t key)
    {
        auto sc = mymath::sincos<T>(angle);
        sc.sin = mymath::abs(sc.sin);
        cache[key] = std::move(sc);
    }

    void dropCache()
    {
        while (cache.size() > size_limit)
            cache.erase(cache.begin());
    }

    auto ensureInCache(float angle)
    {
        const auto key = getKey(angle);
        if (!cache.count(key))
            calcAndCache(angle, key);
        return key;
    }

public:

    T sin(T angle)
    {
        const T val = cache.at(ensureInCache(angle)).sin; //using at to get exception - extra check
        dropCache();
        return val * ((angle < 0) ? -1 : 1);
    }

    T cos(T angle)
    {
        const T val = cache.at(ensureInCache(angle)).cos; //using at to get exception - extra check
        dropCache();
        return val;
    }

    auto sinccos(T angle)
    {
        const auto r = cache.at(ensureInCache(angle));
        dropCache();
        return r;
    }
};

