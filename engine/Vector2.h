//
// Created by alex on 6/27/15.
//

#pragma once

/* Includes - STL */
#include "my_math.h"
#include "strfmt.h"

#include <string>
/* Includes - SFML */

#include "engine/my_math.h"

#include <boost/numeric/ublas/io.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/vector.hpp>

#include <array>
#include <numbers>

template <typename T>
struct VStorage
{
    T x{0};
    T y{0};

    VStorage() = default;
    VStorage(T x, T y) :
        x(x),
        y(y)
    {
    }

    VStorage operator-(const VStorage &right) const
    {
        return VStorage(x - right.x, y - right.y);
    }

    VStorage operator+(const VStorage &right) const
    {
        return VStorage(x + right.x, y + right.y);
    }

    VStorage &operator*=(T scale)
    {
        x *= scale;
        y *= scale;
        return *this;
    }
    VStorage operator/(T data) const
    {
        return VStorage(x / data, y / data);
    }
    VStorage &operator/=(T data)
    {
        x /= data;
        y /= data;
        return *this;
    }
    VStorage operator*(T data) const
    {
        return VStorage(x * data, y * data);
    }
};
template <typename T, typename Scalar>
VStorage<T> operator*(Scalar left, const VStorage<T> &right)
{
    return VStorage<T>(static_cast<T>(left * right.x), static_cast<T>(left * right.y));
}

template <typename T, typename Scalar>
VStorage<T> operator*(const VStorage<T> &left, Scalar right)
{
    return VStorage<T>(static_cast<T>(left.x * right), static_cast<T>(left.y * right));
}

template <typename T>
VStorage<T> operator+(const VStorage<T> &left, const VStorage<T> &right)
{
    return VStorage<T>(left.x + right.x, left.y + right.y);
}

constexpr int FLOAT_PREC = 6;
constexpr int DOUBLE_PREC = 12;
constexpr double PI = 3.14159265358979323846;

/*
 * a c e
 * b d f
 * 0 0 1
 */

template <typename T, int precision>
class Vector2
{
  public:
    using trans_matrix_t = boost::numeric::ublas::matrix<T>;
    using value_t = T;

    inline static constexpr T pi = std::numbers::pi_v<T>;
    inline static constexpr int kPrecision = precision;

  protected:
    VStorage<T> orig;

  public:
    Vector2() = default;
    Vector2(const std::string &X, const std::string &Y) :
        orig((T)std::stod(X), (T)std::stod(Y))
    {
    }
    Vector2(T X, T Y) :
        orig(X, Y)
    {
    }
    virtual ~Vector2() = default;

    Vector2(const VStorage<T> &c) :
        orig(c)
    {
    }

    const auto &operator()() const
    {
        return get();
    }

    const auto &get() const
    {
        return orig;
    }

    T x() const
    {
        return orig.x;
    }

    T y() const
    {
        return orig.y;
    }

    Vector2<T, precision> &operator%=(T theta)
    {
        /*
         * Rotate and assign operator
         * @float       Theta for rotating against
         * ->VECTOR2    this
         */
        const auto sinCos = mymath::sincos(theta);
        orig.x = orig.x * sinCos.cos - orig.y * sinCos.sin;
        orig.y = orig.x * sinCos.sin + orig.y * sinCos.cos;
        return *this;
    }

    Vector2<T, precision> mirror(Vector2<T, precision> onPoint) const
    {
        Vector2<T, precision> res = orig - onPoint.get();
        res.orig.x = -res.orig.x;
        res.orig.y = -res.orig.y;

        return res.get() + onPoint.get();
    }
    static trans_matrix_t getIdentity()
    {
        return boost::numeric::ublas::identity_matrix<T>(3, 3);
    }

    static trans_matrix_t getZeroMatrix()
    {
        return boost::numeric::ublas::zero_matrix<T>(3, 3);
    }

    void translate(const trans_matrix_t &matrix)
    {
        using namespace boost::numeric::ublas;
        vector<T> v(3);
        v[0] = x();
        v[1] = y();
        v[2] = 1;
        v = prod(v, trans(matrix));
        orig.x = v[0];
        orig.y = v[1];
    }

    Vector2<T, precision> operator%(T theta) const
    {
        Vector2<T, precision> v(orig);
        v %= theta;
        return v;
    }

    Vector2<T, precision> norm() const
    {
        return orig / this->mag();
    }

    T dot(const Vector2<T, precision> &otr) const
    {
        return orig.x * otr.x() + orig.y * otr.y();
    }

    T mag() const
    {
        return std::hypot(x(), y());
    }

    T ang() const
    {
        return std::atan2(y(), x());
    }

    const std::string to_str() const
    {
        return stringfmt("[%f, %f] \n", x(), y());
    }

    bool operator==(const Vector2<T, precision> &c) const
    {
        bool ex = mymath::almost_equal<T>(x(), c.x());
        bool ey = mymath::almost_equal<T>(y(), c.y());
        return ex && ey;
    }
};

using Vector2f = Vector2<float, FLOAT_PREC>;

template <class T>
T deg2rad(T deg)
{
    return deg * static_cast<T>(PI) / static_cast<T>(180);
}
