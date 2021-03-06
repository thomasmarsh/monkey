#include "moves.h"

#include "support/catch.hpp"

TEST_CASE("construct a state", "[state]") {
    State s(4);
    s.init();
    REQUIRE(s.size() == NUM_CARDS);
}

TEST_CASE("perform a move", "[state]") {
    State s(4);
    s.init();
    Moves m(s);
    REQUIRE(!m.moves.empty());
    s.perform(m.moves[0]);
}

TEST_CASE("perform a round", "[state]") {
    State s(4);
    s.init();

    for (int i=0; i < 4; ++i) {
        Moves m(s);
        REQUIRE(!m.moves.empty());
        const auto &p = s.current();
        s.perform(m.moves[0]);
        p.visible.print();
    }
    REQUIRE(s.challenge.round.finished());
}

TEST_CASE("perform a challenge", "[state]") {
    State s(4);
    s.init();

    size_t r = 0;
    while (!s.challenge.finished()) {
        while (!s.challenge.round.finished()) {
            Moves m(s);
            REQUIRE(!m.moves.empty());
            auto &move = m.moves[urand(m.moves.size())];
            s.perform(move);
        }
        s.challenge.round.reset();
        s.validateCards();
        ++r;
    }
}

TEST_CASE("play a game", "[state]") {
    State s(4);
    s.init();
    size_t c = 0;
    while (!s.challenge.round.game_over) {
        LOG("");
        LOG("CHALLENGE #{}", c);
        LOG("");
        size_t r = 0;
        while (!s.challenge.finished()) {
            LOG("ROUND <{}>", r);
            while (!s.challenge.round.finished()) {
                Moves m(s);
                REQUIRE(!m.moves.empty());
                const auto &p = s.current();
                for (const auto &move : m.moves) {
                    LOG("    - {}", to_string(move));
                }
                auto &move = m.moves[urand(m.moves.size())];
                s.perform(move);
            }
            s.challenge.round.reset();
            s.validateCards();
            ++r;
        }
        s.checkReset();
        ++c;
    }
}
