#pragma once

#include "log.h"

#include <vector>
#include <algorithm>
#include <string>

constexpr size_t MAX_DOUBLE_INDICES = 15;

using IndexList = std::vector<size_t>;
using PermutationList = std::vector<std::pair<size_t, IndexList>>;

extern PermutationList DOUBLE_INDICES;

extern void BuildDoubleIndices();

inline size_t NumDoubleIndices(size_t n) {
    // This is the same as the binomial coefficient * 2.

    assert(n < MAX_DOUBLE_INDICES);
    size_t i = 0;
    while (true) {
        auto &entry = DOUBLE_INDICES[i];
        if (entry.first >= n) {
            return i;
        }
        ++i;
    }
    ERROR("n too high");
}

inline std::vector<IndexList> DoubleIndices(size_t n) {
    assert(n < MAX_DOUBLE_INDICES);
    auto indices = std::vector<IndexList>();
    size_t i=0;
    while (true) {
        auto &entry = DOUBLE_INDICES[i];
        if (entry.first >= n) {
            return indices;
        }
        indices.push_back(entry.second);
        ++i;
    }
    ERROR("n too high");
}

// ---------------------------------------------------------------------------

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
