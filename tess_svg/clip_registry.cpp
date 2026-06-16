#include "clip_registry.hpp"

#include "tess_svg/GlDefs.h"
#include "tess_svg/SvgProcessor.h"
#include "tess_svg/geometry_engine.hpp"
#include "tess_svg/processing_data.hpp"

#include <optional>
#include <string>
#include <utility>
#include <vector>

ClipRegistry::ClipRegistry(const std::vector<SvgGroup> &defs)
{
    buildCache(defs);
}

std::optional<std::vector<Loops>> ClipRegistry::findClipperById(const std::string &id) const
{
    auto it = cache_.find(id);
    if (it != cache_.end())
    {
        return it->second;
    }
    return std::nullopt;
}

std::string ClipRegistry::extractIdFromUrl(const std::string &url_attr)
{
    // Ожидаем формат "url(#ID)"
    if (url_attr.size() > 5 && url_attr.starts_with("url(") && url_attr[4] == '#')
    {
        return url_attr.substr(5, url_attr.length() - 6);
    }
    return "";
}

std::vector<Loops> ClipRegistry::groupToUnifiedIslands(const SvgGroup &group) const
{
    // Собираем все "сырые" контуры (islands/contours) из элементов группы.
    const std::vector<Loops> all_elements_loops = getLoopsFromGroup(group);
    if (all_elements_loops.empty())
    {
        return {};
    }
    // geometry_engine::unionShapes(std::vector<Loops>)
    // Принимает список островов и возвращает очищенный std::vector<Loops>,
    // где каждый элемент — это один остров с его границами и дырками.
    return geometry_engine::unionShapes(all_elements_loops);
}

void ClipRegistry::buildCache(const std::vector<SvgGroup> &defs)
{
    for (const auto &group : defs)
    {
        const std::string &gid = group.id();
        if (gid.empty())
        {
            continue;
        }
        std::vector<Loops> islands = groupToUnifiedIslands(group);
        if (!islands.empty())
        {
            cache_[gid] = std::move(islands);
        }
    }
}
