#pragma once

#include "tess_svg/GlDefs.h"

#include <vector>

namespace geometry_engine {
/**
 * @brief Merges a set of contours (Loops) into the minimum number of non-intersecting cycles.
 * Used for geometry simplification ("Inkscape Mode") and group collapsing.
 * @return vector of Loops. Each element is dedicated island (Loops). Each element of the island
 * (Loops) is contour + holes.
 */
[[nodiscard]]
std::vector<Loops> unionShapes(const std::vector<Loops> &shapes);

/// @brief If loops contain even-odd path it must be resolved prior passing to unionShapes.
Loops resolveEvenOddInternalTopology(Loops raw_loops);

/**
 * @brief Intersects the main contour with a list of masks (Clipping).
 * If the masks are represented as a set of loops, they will be processed as a single object.
 */
[[nodiscard]]
Loops intersectWithMasks(const Loops &subject, const std::vector<Loops> &masks);
} // namespace geometry_engine
