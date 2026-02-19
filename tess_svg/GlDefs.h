//
// Created by alex on 7/14/15.
//

#ifndef TESSVG_GLDEFS_H
#define TESSVG_GLDEFS_H

#include "../engine/Vector2.h"

#include <GL/glu.h>

#include <algorithm>
#include <cmath>
#include <vector>

constexpr int GL_PREC = 17;

using used_vectors_type = GLdouble;

using GlVertex = Vector2<used_vectors_type, GL_PREC>;

using Vertexes = std::vector<GlVertex>;
using Loops = std::vector<Vertexes>;
using GlContour = std::vector<used_vectors_type>;
using GlPolygon = std::vector<GlContour>;

#endif // TESSVG_GLDEFS_H
