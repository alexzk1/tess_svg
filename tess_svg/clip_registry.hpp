#pragma once

#include "tess_svg/GlDefs.h"
#include "tess_svg/processing_data.hpp"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @brief High-performance registry for SVG clipping paths.
 * A mask is represented as a std::vector<Loops>, where each Loops is an "Island"
 * (one outer boundary + N holes).
 */
class ClipRegistry
{
  public:
    /// @brief Constructs the registry and builds the cache from SvgWorld::defs definitions.
    explicit ClipRegistry(const std::vector<SvgGroup> &defs);

    /**
     * @brief Finds the unified geometry associated with an ID.
     * @return std::optional containing a collection of islands (std::vector<Loops>) if found.
     */
    [[nodiscard]]
    std::optional<std::vector<Loops>> findClipperById(const std::string &id) const;

    /// @brief Extracts the ID from a "url(#id)" string.
    static std::string extractIdFromUrl(const std::string &url_attr);

    [[nodiscard]]
    bool empty() const
    {
        return cache_.empty();
    }

  protected:
    /**
     * @brief Converts a group and all its elements into a collection of non-overlapping islands.
     * Preserves holes within each island by using unionShapes.
     */
    std::vector<Loops> groupToUnifiedIslands(const SvgGroup &group) const;

  private:
    void buildCache(const std::vector<SvgGroup> &defs);

    /// @brief Map of ID to a collection of islands (the mask).
    std::unordered_map<std::string, std::vector<Loops>> cache_;
};
