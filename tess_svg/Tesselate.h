//
// Created by alex on 7/14/15.
//

#ifndef TESSVG_TESSELACT_H
#define TESSVG_TESSELACT_H

#include "GlDefs.h"

/**
 * @brief Geomtry generation mode.
 */
enum class TessMode {
    /// @brief Full triangulation.
    Triangles,
    /// @brief Contour only (for example collisions).
    Contours,
};

class Tesselate
{
  public:
  private:
    Vertexes tlist;

  public:
    const Vertexes &process(const Loops &vertexes, TessMode mode);
    [[nodiscard]]
    const Vertexes &getTesselated() const;
};

#endif // TESSVG_TESSELACT_H
