#include "round.h"
#include "log.h"

#include "support/catch.hpp"

TEST_CASE("initial state good", "[round]") {
    Round r(4);
    REQUIRE(!r.finished());
    REQUIRE(!r.challengeFinished());
}

TEST_CASE("can step", "[round]") {
    Round r(4);
    r.current = 0;
    r.step();
    REQUIRE(r.current == 1);
}

TEST_CASE("can step with wrap", "[round]") {
    Round r(4);
    r.current = 3;
    r.step();
    REQUIRE(r.current == 0);
}

TEST_CASE("all plays end round", "[round]") {
    for (int i=2; i < 5; ++i) {
        Round r(i);
        for (int j=0; j < i; ++j) {
            REQUIRE(!r.finished());
            REQUIRE(!r.challengeFinished());
            r.step();
        }
        REQUIRE(r.finished());
        REQUIRE(!r.challengeFinished());
    }
}

TEST_CASE("passing ends round and challenge", "[round]") {
    Round r(4);
    for (int i=0; i < 4; ++i) {
        REQUIRE(!r.finished());
        r.pass();
        r.step();
    }
    REQUIRE(r.finished());
    REQUIRE(r.challengeFinished());
}

TEST_CASE("conceding ends round", "[round]") {
    Round r(4);
    for (int i=0; i < 3; ++i) {
        REQUIRE(!r.finished());
        r.concede();
        r.step();
    }
    REQUIRE(r.finished());
    REQUIRE(r.challengeFinished());
}

TEST_CASE("ended challenge needs hard reset", "[round]") {
    Round r(2);
    for (int i=0; i < 2; ++i) {
        r.pass();
        r.step();
    }
    REQUIRE(r.finished());
    REQUIRE(r.challengeFinished());
    r.reset();
    REQUIRE(r.finished());
    REQUIRE(r.challengeFinished());
    r.hardReset();
    REQUIRE(!r.finished());
    REQUIRE(!r.challengeFinished());
}

TEST_CASE("fuzz test round end", "[round]") {
    for (int z=0; z < 10000; ++z) {
        size_t p = urand(3)+2;

        Round r(p);
        for (int i=0; i < p; ++i) {
            REQUIRE(!r.finished());
            switch (urand(2)) {
            case 0: r.pass(); break;
            case 1: break;
            }
            r.step();
        }
        REQUIRE(r.finished());
    }
}

TEST_CASE("reset", "[round]") {
    Round r(4);
    for (int i=0; i < 4; ++i) { r.step(); }
    REQUIRE(r.finished());
    r.reset();
    for (int i=0; i < 4; ++i) {
        REQUIRE(!r.finished());
        r.step();
    }
    REQUIRE(r.finished());
    REQUIRE(!r.challengeFinished());
}

TEST_CASE("concession persists across reset", "[round]") {
    Round r(4);

    bool done = false;
    size_t c = 0;
    for (int j=4; j != 1; --j) {
        for (int i=0; i < j; ++i) {
            REQUIRE(!r.finished());
            REQUIRE(!r.challengeFinished());
            if (i == 0) {
                r.concede();
                ++c;
                if (c == 3) {
                    break;
                }
            }
            r.step();
        }
        REQUIRE(r.finished());
        if (r.challengeFinished()) {
            REQUIRE(!done);
            done = true;
        }
        r.reset();
    }
    REQUIRE(r.finished());
    REQUIRE(r.challengeFinished());
}

TEST_CASE("can reuse round across challenges", "[round]") {
    int p = 4;
    Round r(p);
    for (int k=0; k < 10; ++k) {
        bool done = false;
        size_t c = 0;
        for (int j=4; j != 1; --j) {
            for (int i=0; i < j; ++i) {
                REQUIRE(!r.finished());
                REQUIRE(!r.challengeFinished());
                if (i == 0) {
                    r.concede();
                    ++c;
                    if (c == 3) {
                        break;
                    }
                }
                r.step();
            }
            REQUIRE(r.finished());
            if (r.challengeFinished()) {
                REQUIRE(!done);
                done = true;
            }
            r.reset();
        }
        REQUIRE(r.finished());
        REQUIRE(r.challengeFinished());
        r.hardReset();
    }
}
