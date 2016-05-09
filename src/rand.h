#pragma once

#include <random>
#include <cstdlib>
#include <cassert>

constexpr size_t ARC4RANDOM_MAX = 0x100000000L;

struct ARC4RNG {
    typedef size_t result_type;
    constexpr static size_t max() { return ARC4RANDOM_MAX; }
    constexpr static size_t min() { return 0; }
    size_t operator()() { return arc4random(); }
};

extern ARC4RNG Arc4RNG;

template <typename T>
inline void Sort(T &v) {
    std::sort(std::begin(v), std::end(v));
}

template <typename T, typename F>
inline void Sort(T &v, F f) {
    std::sort(std::begin(v), std::end(v), f);
}

template <typename T>
inline void Shuffle(T &v) {
    std::shuffle(std::begin(v), std::end(v), Arc4RNG);
}

template <typename I>
inline void Shuffle(I begin, I end) {
    std::shuffle(begin, end, Arc4RNG);
}

inline size_t urand(size_t n) {
    assert(n < ARC4RANDOM_MAX);
    return arc4random_uniform((unsigned int) n);
}

inline float RandFloat() {
    return (float)arc4random() / ARC4RANDOM_MAX;
}

template <typename I>
inline I Choice(I begin, I end) {
    return begin + urand(std::distance(begin, end));
}
