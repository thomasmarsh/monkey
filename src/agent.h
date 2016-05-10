#pragma once

#include "state.h"
#include "moves.h"

template <typename A>
void Rollout(State &s, A &agent) {
    while (!s.gameOver()) {
        agent.move(s);

        // TODO: these shouldn't need to reach so deep
        if (s.challenge.round.finished()) {
            s.challenge.round.reset();
        }
    }
}

struct RandomAgent {
    std::string name() const { return "Random"; }

    void move(State &s) {
        Moves moves(s);
        assert(!moves.moves.empty());
        auto &move = moves.moves[urand(moves.moves.size())];
        if (move.action == Action::CONCEDE) {
            move = Move::Pass();
        }
        s.perform(&move);
    }
};
