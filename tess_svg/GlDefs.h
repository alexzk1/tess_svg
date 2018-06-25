//
// Created by alex on 7/14/15.
//

#ifndef TESSVG_GLDEFS_H
#define TESSVG_GLDEFS_H

#include <GL/glu.h>
#include "../engine/Vector2.h"
#include <vector>
#include <cmath>
#include <algorithm>


constexpr int GL_PREC = 17;

typedef GLdouble used_vectors_type;


typedef Vector2<used_vectors_type, GL_PREC> GlVertex;


typedef std::vector<GlVertex>  Vertexes;
typedef std::vector<Vertexes>  Loops;
typedef std::vector<used_vectors_type>  GlContour;
typedef std::vector<GlContour> GlPolygon;


#endif //TESSVG_GLDEFS_H
