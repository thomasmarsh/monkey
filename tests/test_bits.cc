#include "bits.h"
#include "log.h"

#include "support/catch.hpp"

struct Result {
    size_t count;
    size_t sum;

    Result() : count(0), sum(0) {}
    Result(size_t c, size_t s) : count(c), sum(s) {}
};

static Result iterate(uint64_t x) {
    Result r;
    for (auto i : EachBit(x)) {
        ++r.count;
        r.sum += i;
    }
    return r;
}

using B64 = Bitset<uint64_t>;
using B8 = Bitset<uint8_t>;

TEST_CASE("iterate over each bit", "[bits]") {
    auto r = iterate(2);
    REQUIRE(r.count > 0);
    REQUIRE(r.sum > 0);

    r = iterate(0);
    REQUIRE(r.count == 0);
    REQUIRE(r.sum == 0);

    r = iterate(0b1001);
    REQUIRE(r.count == 2);
    REQUIRE(r.sum == 3);
}

TEST_CASE("bitsets count", "[bits]") {
    struct Case {
        uint64_t x;
        Result expect;
    };

    for (auto c : std::vector<Case> { { 0b000, { 0, 0 } },
                                      { 0b001, { 1, 0 } },
                                      { 0b010, { 1, 1 } },
                                      { 0b011, { 2, 1 } },
                                      { 0b100, { 1, 2 } },
                                      { 0b111, { 3, 3 } },
                                      { 0xFFFFFFFFFFFFFFFFULL, { 64, 2016 } },
                                    }) {
        auto r = iterate(c.x);
        REQUIRE(r.count == c.expect.count);
        REQUIRE(r.sum == c.expect.sum);
        REQUIRE(B64(c.x).count() == c.expect.count);
    }
}

TEST_CASE("bitset sets bits", "[bits]") {
    B64 x;
    REQUIRE(x == 0);
    x.set(2);
    REQUIRE(x == 0b100);
}

TEST_CASE("bitset sets all bits", "[bits]") {
    B64 x;
    for (size_t i=0; i < sizeof(x)*8; ++i) {
        x.set(i);
    }
    REQUIRE(x == 0xFFFFFFFFFFFFFFFFULL);
}

TEST_CASE("bitset clears bits", "[bits]") {
    B64 x = 0b1100;
    REQUIRE(x == 0b1100);
    x.clear(2);
    REQUIRE(x == 0b1000);
}

TEST_CASE("bitset clears all bits", "[bits]") {
    B64 x = 0xFFFFFFFFFFFFFFFFULL;
    for (size_t i=0; i < sizeof(x)*8; ++i) {
        x.clear(i);
    }
    REQUIRE(x == 0);

    x.set(63);
    REQUIRE(x == 0x8000000000000000ULL);
    x.clear(63);
    REQUIRE(x == 0);
}

TEST_CASE("bitset detects empty", "[bits]") {
    B64 x;
    REQUIRE(x.empty());
    x.set(0);
    REQUIRE(!x.empty());
    x.clear(0);
    REQUIRE(x.empty());
}

TEST_CASE("bitset can drop bits", "[bits]") {
    B64 x = 0b11110010;
    x.drop(0);
    REQUIRE(x == 0b01111001);
    x.drop(3);
    REQUIRE(x == 0b00111001);

    x = 0;
    x.set(63);
    REQUIRE(x == 0x8000000000000000ULL);
    x.drop(63);
    REQUIRE(x == 0);

    x.set(0);
    REQUIRE(x == 1);
    x.drop(0);
    REQUIRE(x.empty());
}

TEST_CASE("bitset can fill", "[bits]") {
    B8 x;
    x.fill(4);
    REQUIRE(x == 0b00001111);

    x = 0;
    x.hfill(4);
    REQUIRE(x == 0b11110000);

    x = 0b10101010;
    x.fill(4);
    REQUIRE(x == 0b10101111);

    x = 0b10101010;
    x.hfill(4);
    REQUIRE(x == 0b11111010);
}

TEST_CASE("bitset can find index", "[bits]") {
    REQUIRE(B8(0b0001).index(0) == 0);
    REQUIRE(B8(0b0011).index(1) == 1);
    REQUIRE(B8(0b0101).index(1) == 2);
    REQUIRE(B8(0b1101).index(2) == 3);
}
