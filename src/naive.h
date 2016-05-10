#pragma once

#include "agent.h"

struct NaiveAgent : Agent {
    std::string name() const override { return "Naive"; }

    int cardValue(const Card &c, const State &s) const {
        if (s.challenge.invert_value) {
            return c.inverted_value;
        }
        return c.face_value;
    }

    Affinity bestSide(const State &s) const {
        int clan = 0;
        int monk = 0;
        for (auto c : s.current().hand.characters) {
            const auto &card = Card::Get(c);
            switch (card.affinity) {
            case Affinity::CLAN: clan += cardValue(card, s); break;
            case Affinity::MONK: monk += cardValue(card, s); break;
            case Affinity::NONE: break;
            }
        }

        if      (clan > monk) { return Affinity::CLAN; }
        else if (monk > clan) { return Affinity::MONK; }

        return Affinity::NONE;
    }

    int valueForMove(const Move &m, const State &s) const {
        if (m.card == Move::null) {
            return 0;
        }
        return cardValue(Card::Get(m.card), s);
    }

    void move(State &s) const override {
        Moves moves(s);

        std::vector<int> values;
        values.reserve(moves.moves.size());

        size_t sum = 0;
        for (const auto &m : moves.moves) {
            auto v = 1+valueForMove(m, s);
            sum += v;
            values.push_back(v);
        }

        auto choice = urand(sum);
        auto move = Move::Pass();

        sum = 0;
        int i = 0;
        for (const auto &m : moves.moves) {
            sum += values[i];
            ++i;
            if (sum >= choice) {
                move = m;
                break;
            }
        }
        s.perform(&move);
    }
};
