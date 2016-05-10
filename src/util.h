#pragma once

#include "log.h"

#include <string>

template <typename T, typename V>
inline bool Contains(const T &c, const V &v) {
    return std::find(std::begin(c), std::end(c), v) != std::end(c);
}

inline std::string EscapeQuotes(const std::string &before) {
    std::string after;
    after.reserve(before.length() + 4);

    for (std::string::size_type i=0; i < before.length(); ++i) {
        switch (before[i]) {
        case '"':
        case '\\': after += '\\';
        default:   after += before[i];
        }
    }

    return after;
}
