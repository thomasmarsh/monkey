#pragma once

#include "state.h"
#include "moves.h"

#include <signal.h>

inline size_t RandomMove(const Moves &m, State &s) {
    assert(!m.moves.empty());
    const size_t i = urand(m.moves.size());
    const auto &move = m.moves[i];
    if (move.isConcede()) {
        s.perform(Move::Pass());
    } else {
        s.perform(move);
    }
    return i;
}

template <typename A>
inline void Rollout(State &s, A &agent) {
    while (!s.gameOver()) {
        if (s.gameOver()) {
            WARN("rollout called on finished game");
            break;
        }

        agent.move(s);
        s.checkReset();
    }
}

struct RandomAgent {
    std::string name() const { return "Random"; }

    void move(State &s) {
        const Moves m(s);
        RandomMove(m, s);
    }
};
