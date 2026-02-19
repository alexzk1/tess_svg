//
// Created by alex on 7/14/15.
//

#ifndef TESSVG_BOUNDS_H
#define TESSVG_BOUNDS_H

#include "GlDefs.h"

template <class T, int precission = 15>
class Bounds
{
  private:
    bool notset;

  public:
    T xmin;
    T xmax;
    T ymin;
    T ymax;

    Bounds()
    {
        reset();
    }

    void reset()
    {
        notset = true;
        xmin = xmax = ymin = ymax = 0;
    }

    T width() const
    {
        return std::abs(xmax - xmin);
    }

    T height() const
    {
        return std::abs(ymax - ymin);
    }

    T scaleFactor() const
    {
        return std::max(width(), height());
    }

    void add_point(T x, T y)
    {
        if (notset)
        {
            xmin = xmax = x;
            ymin = ymax = y;
        }
        else
        {
            xmin = std::min(xmin, x);
            ymin = std::min(ymin, y);

            xmax = std::max(xmax, x);
            ymax = std::max(ymax, y);
        }
        notset = false;
    }

    void add_bounds(const Bounds &other)
    {
        if (!other.notset)
        {
            if (notset)
                *this = other;
            else
            {
                xmin = std::min(xmin, other.xmin);
                ymin = std::min(ymin, other.ymin);

                xmax = std::max(xmax, other.xmax);
                ymax = std::max(ymax, other.ymax);
            }
            notset = false;
        }
    }

    Vector2<T, precission> get_center() const
    {
        return Vector2<T, precission>((xmin + xmax) / 2., (ymin + ymax) / 2.);
    }

    void offset(T x, T y)
    {
        if (!notset)
        {
            xmin += x;
            xmax += x;
            ymin += y;
            ymax += y;
        }
    }
};

typedef Bounds<GLdouble, GL_PREC> GlBounds;

#endif // TESSVG_BOUNDS_H
