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

/// @brief Final step which combines multiply polylines of 1 object (Loops) into single outer
/// polygon of this 1 object (physical / collision shape).
/// @note: Internal holes are integrated into the outer boundary to provide a single continuous
/// vertex array, preventing topological discontinuities in downstream physics/rendering engines.
class Tesselate
{
  public:
  private:
    Polyline tlist;

  public:
    /// @brief Combine polylines into outer polygon.
    /// @param src_polyline 1 or more polylines which represent 1(sub)-object.
    /// @param mode TessMode of output.
    /// @returns Single Polyline which is bounding polygon (physical / collision shape).
    [[nodiscard]]
    const Polyline &process(const Loops &src_polylines, TessMode mode);
    [[nodiscard]]
    const Polyline &getTesselated() const;
};

#endif // TESSVG_TESSELACT_H
