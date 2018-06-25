#ifndef SF_POLYGON_H
#define SF_POLYGON_H
#include "SFML/Graphics/ConvexShape.hpp"
#include <iterator>
#include <cassert>

class SfPolygon : public sf::ConvexShape
{
public:
    explicit SfPolygon(size_t points = 0):
        sf::ConvexShape(points)
    {
    }

    SfPolygon(const std::vector<float>& points_pairs)
    {
        initFromVector(points_pairs);
    }

    SfPolygon(const std::vector<float>& points_pairs, float originX, float originY)
    {
        initFromVector(points_pairs);
        setOrigin(originX, originY);
    }

    void initFromVector(const std::vector<float>& points_pairs)
    {
        assert(points_pairs.size() % 2 == 0); //must be pairs of x,y,x2,y2,x3,y3 etc
        size_t psize = points_pairs.size() / 2;
        setPointCount(psize);
        for (size_t i = 0; i < psize; ++i)
        {
            setPoint(i, {points_pairs.at(i * 2 + 0), points_pairs.at(i * 2 + 1)});
        }
    }
};

#endif // SF_POLYGON_H
