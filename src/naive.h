#pragma once

#include "agent.h"

struct NaivePlayer : Agent {
    string name() const override { return "Naive"; }

    Affinity bestSide() {
        int clan = 0;
        int monk = 0;
        for (auto c : hand.characters) {
            const auto &card = CARD_TABLE[c];
            switch (card.affinity) {
            case CLAN: clan += card.value; break;
            case MONK: monk += card.value; break;
            case NONE: break;
            }
        }

        if      (clan > monk) { return CLAN; }
        else if (monk > clan) { return MONK; }

        return NONE;
    }

    Move selectMove(GameState::Ptr state) {
        auto moves = state->getCurrentMoveList();

        vector<int> values;
        values.reserve(moves.size());

        size_t sum = 0;
        for (const auto &m : moves) {
            auto v = 1+valueForMove(m, state->flags);
            sum += v;
            values.push_back(v);
        }

        auto choice = urand(sum);
        auto move = Move::NullMove();

        sum = 0;
        int i = 0;
        for (const auto &m : moves) {
            sum += values[i];
            ++i;
            if (sum >= choice) {
                move = m;
                break;
            }
        }
        return move;
    }

    Move move(GameState::Ptr state) override {
        TRACE();
        return selectMove(state);
    }
};
