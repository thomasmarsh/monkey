#pragma once

#include "state.h"
#include "moves.h"

struct Agent {
    virtual std::string name() const = 0;
    virtual void move(State &s) const = 0;
};

struct RandomAgent : Agent {
    std::string name() const override { return "Random"; }

    void move(State &s) const override {
        Moves moves(s);
        assert(!moves.moves.empty());
        auto &move = moves.moves[urand(moves.moves.size())];
        if (move.action == Action::CONCEDE) {
            move = Move::Pass();
        }
        s.perform(&move);
    }
};
