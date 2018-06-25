//
// Created by alex on 6/27/15.
//

#ifndef BEAMMER_VECTOR2_H
#define BEAMMER_VECTOR2_H
#pragma once

/* Includes - STL */
#include <string>

/* Includes - SFML */
#include <SFML/System/Vector2.hpp>
#include "sincos_cached.h"
#include <array>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/io.hpp>

constexpr int FLOAT_PREC  = 6;
constexpr int DOUBLE_PREC = 12;
constexpr double PI = 3.14159265358979323846;




/*
 * a c e
 * b d f
 * 0 0 1
*/

template <typename T, int precission>
class Vector2
{
public:
    using trans_matrix_t = boost::numeric::ublas::matrix<T>;
protected:
    static constexpr T pi = (T)PI;
    sf::Vector2<T> orig;
public:
    static sincos_cached<T, precission> sincos;

    Vector2() = default;
    Vector2(const std::string& X, const std::string& Y): orig((T)std::stod(X), (T)std::stod(Y))
    {

    }
    Vector2(T X, T Y) : orig(X, Y)
    {
    }


    Vector2(const sf::Vector2<T>& c) : orig(c)//no need in explicit here, I want easy mix of Vector2 and sf::Vector2
    {
    }

    virtual ~Vector2() = default;


    const sf::Vector2<T>& operator()() const
    {
        return get();
    }

    const sf::Vector2<T>& get() const
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

    Vector2<T, precission>& operator%=(T theta)
    {
        /*
        * Rotate and assign operator
        * @float       Theta for rotating against
        * ->VECTOR2    this
        */
        T cs = sincos.cos(theta);
        T sn = sincos.sin(theta);

        T px = orig.x * cs - orig.y * sn;
        T py = orig.x * sn + orig.y * cs;

        orig.x = px;
        orig.y = py;

        return *this;
    }

    Vector2<T, precission> mirror(Vector2<T, precission> onPoint) const
    {
        Vector2<T, precission> res = orig - onPoint.get();
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

    void translate(const trans_matrix_t& matrix)
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

    Vector2<T, precission> operator%(T theta) const
    {
        Vector2<T, precission> v(orig);
        v %= theta;
        return v;
    }

    Vector2<T, precission> norm() const
    {
        return orig / this->mag();
    }

    T dot(const Vector2<T, precission>& otr) const
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
        char tmpbuf[256];
        sprintf(tmpbuf, "[%f, %f] \n", x(), y());
        return std::string(tmpbuf);
    }

    bool operator == (const Vector2<T, precission>& c) const
    {
        bool ex = almost_equal<T, precission>(x(), c.x());
        bool ey = almost_equal<T, precission>(y(), c.y());
        return ex && ey;
    }
};

using Vector2f = Vector2<float, FLOAT_PREC>;


#include <vector>
#include <SFML/Graphics.hpp>

template<sf::PrimitiveType TPrimitive> struct VertexVector : public std::vector<sf::Vertex>, public sf::Drawable
{
    using std::vector<sf::Vertex>::vector;
    inline void draw(sf::RenderTarget& mRenderTarget, sf::RenderStates mRenderStates) const override
    {
        mRenderTarget.draw(&this->operator [](0), this->size(), TPrimitive, mRenderStates);
    }
};


template <class T, int precission>
sincos_cached<T, precission> Vector2<T, precission>::sincos;

template <class T>
T deg2rad(T deg)
{
    return deg * (T)PI / 180;
}

#endif //BEAMMER_VECTOR2_H
