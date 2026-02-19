#pragma once

#include <algorithm>
#include <cctype>
#include <string>

inline std::string toLower(std::string src)
{
    std::transform(src.begin(), src.end(), src.begin(), ::tolower);
    return src;
}
