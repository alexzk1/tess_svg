#pragma once

#include <cctype>
#include <string>
#include <algorithm>

inline std::string toLower(std::string src)
{
    std::transform(src.begin(), src.end(), src.begin(), ::tolower);
    return src;
}
