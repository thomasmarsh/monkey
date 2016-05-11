#pragma once

#include <cstdint>
#include <cstddef>

// Unfortunately the intrinic is too smart for our needs
//#define LOG2(x)   ((sizeof(x)*8-1)-__builtin_clzll((uint64_t)(x)))

inline uint64_t LOG2(uint64_t x) {
    uint64_t y;
    asm ( "\tbsrq %1, %0\n"
          : "=r"(y)
          : "r" (x)
        );
    return y;
}

#define POPCNT(x) __builtin_popcountll(x)

struct EachBit {
    uint64_t x;

    EachBit(uint64_t _x) : x(_x) {}

    struct Iterator {
        uint64_t x;
        size_t y;

        Iterator(uint64_t _x) : x(_x), y(LOG2(x & (-x))) {}

        const size_t operator*() const { return y; }

        size_t operator++() {
            x &= x-1ULL;
            y = LOG2(x & (-x));
            return y;
        }

        size_t operator +(size_t n) {
            for (size_t i=0; i < n; ++i) {
                ++(*this);
            }
            return this->y;
        }

        bool operator!=(const Iterator &other) const {
            return x != other.x;
        }
    };

    Iterator begin()       { return Iterator(x); }
    Iterator end()         { return Iterator(0); }
    Iterator begin() const { return Iterator(x); }
    Iterator end()   const { return Iterator(0); }
};

template <typename T>
struct Bitset {
    T v;

    Bitset()     : v(0)  {}
    Bitset(T _v) : v(_v) {}

    operator T() const { return v; }
    bool operator [] (size_t i)  { return test(i); }

    T      ones(size_t i)  const { return (T(1) << i)-T(1); }
    void   set(size_t i)         { v |= (1ULL << i); }
    void   clear(size_t i)       { v &= ~(1ULL << i); }
    bool   test(size_t i)  const { return v & (1ULL << i); }
    bool   empty()         const { return !v; }
    size_t count()         const { return POPCNT(v); }
    void   fill(size_t n)        { v |= ones(n); }
    void   hfill(size_t n)       { v |= T(-1) << (sizeof(T)*8-n); }

    void drop(size_t i) {
        T high = v & (T(-1) << i);
        v = (v & ~high) | ((high >> (i+1)) << i);
    }

    // returns the index of the nth set bit
    size_t index(size_t i) {
        return EachBit::Iterator(v)+i;
    }
};

