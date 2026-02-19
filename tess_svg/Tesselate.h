//
// Created by alex on 7/14/15.
//

#ifndef TESSVG_TESSELACT_H
#define TESSVG_TESSELACT_H

#include "GlDefs.h"

#include <functional>
#include <memory>

class Tesselate
{
  public:
  private:
    Vertexes tlist;

  public:
    const Vertexes &process(const Loops &vertexes, bool contourOnly);
    const Vertexes &getTesselated() const;
};

#endif // TESSVG_TESSELACT_H
