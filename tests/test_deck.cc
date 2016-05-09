#include "deck.h"
#include "log.h"

#include "support/catch.hpp"

TEST_CASE("initial deck state good", "[deck]") {
    Deck d;
    d.populate();
    REQUIRE(d.size() == NUM_CARDS);
}

TEST_CASE("deck shuffle", "[deck]") {
    Deck d;
    d.populate();
    auto clone = d.clone();
    REQUIRE(*clone == d);
    clone->shuffle();
    REQUIRE(*clone != d);
}
