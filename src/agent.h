#pragma once

#include "state.h"
#include "moves.h"

#include <signal.h>

inline size_t RandomMove(const Moves &m, State &s) {
    assert(!m.moves.empty());
    size_t i = urand(m.moves.size());
    auto move = m.moves[i];
    if (move.action == Action::CONCEDE) {
        move = Move::Pass();
    }
    s.perform(&move);
    return i;
}

inline void ForwardState(State &s) {
    // TODO: these shouldn't need to reach so deep
    if (s.challenge.round.finished()) {
        s.challenge.round.reset();
    }
    if (s.challenge.finished()) {
        s.reset();
    }
}


template <typename A>
inline void Rollout(State &s, A &agent) {
    while (!s.gameOver()) {
        ForwardState(s);

        if (s.gameOver()) {
            break;
        }

        agent.move(s);

        ForwardState(s);
    }
}

struct RandomAgent {
    std::string name() const { return "Random"; }

    void move(State &s) {
        Moves m(s);
        RandomMove(m, s);
    }
};
