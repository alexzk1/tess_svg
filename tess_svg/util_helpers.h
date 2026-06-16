#pragma once

#include <algorithm>
#include <cctype>
#include <iterator>
#include <string>
#include <vector>

inline std::string toLower(std::string src)
{
    std::transform(src.begin(), src.end(), src.begin(), ::tolower);
    return src;
}

template <typename taType>
void appendVectors(std::vector<taType> &appendTo, std::vector<taType> what)
{
    appendTo.insert(appendTo.end(), std::make_move_iterator(what.begin()),
                    std::make_move_iterator(what.end()));
}
