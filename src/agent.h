#pragma once

#include "state.h"
#include "moves.h"

struct Agent {
    virtual std::string name() const = 0;
    virtual Move move(const State &s) const = 0;
};

struct RandomAgent : Agent {
    std::string name() const override { return "Random"; }

    Move move(const State &s) const override {
        Moves moves(s);
        assert(!moves.moves.empty());
        auto move = moves.moves[urand(moves.moves.size())];
        if (move.action == Action::CONCEDE) {
            return Move::Pass();
        }
        return move;
    }
};
