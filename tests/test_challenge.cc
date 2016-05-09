#include "challenge.h"
#include "log.h"

#include "support/catch.hpp"

TEST_CASE("initial challenge state good", "[challenge]") {
    Challenge c(4);
    REQUIRE(!c.finished());
}

TEST_CASE("detect end of challenge", "[challenge]") {
    SET_LOG_LEVEL(info);
    Challenge c(4);
    for (int i=0; i < 4; ++i) {
        c.round.pass();
        c.step();
    }
    REQUIRE(c.finished());
}

TEST_CASE("reset after end of challenge", "[challenge]") {
    Challenge c(4);
    for (int i=0; i < 3; ++i) {
        c.round.concede();
        c.step();
    }
    REQUIRE(c.finished());

    c.reset();

    REQUIRE(!c.finished());
    for (int i=0; i < 4; ++i) {
        c.round.pass();
        c.step();
    }
    REQUIRE(c.finished());
}
