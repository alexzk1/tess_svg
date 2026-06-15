//
// Created by alex on 7/14/15.
//

#ifndef TESSVG_TESSELACT_H
#define TESSVG_TESSELACT_H

#include "GlDefs.h"

#include <cstdint>

/**
 * @brief Geomtry generation mode.
 */
enum class TessMode : std::uint8_t {
    /// @brief Full triangulation.
    Triangles,
    /// @brief Contour only (for example collisions).
    Contours,
};

class Tesselate
{
  public:
  private:
    Polyline tlist;

  public:
    const Polyline &process(const Loops &src_polylines, TessMode mode);
    [[nodiscard]]
    const Polyline &getTesselated() const;
};

#endif // TESSVG_TESSELACT_H
