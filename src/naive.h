#pragma once

#include "state.h"
#include "moves.h"

struct NaiveAgent {
    std::string name() const { return "Naive"; }

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
        if (m.index() == Move::null) {
            return 0;
        }
        return cardValue(Card::Get(m.index()), s);
    }

    void move(State &s) {
        const Moves moves(s);

        std::vector<int> values;
        values.reserve(moves.moves.size());

        size_t sum = 0;
        for (const auto &m : moves.moves) {
            auto v = 1+valueForMove(m, s);
            sum += v;
            values.push_back(v);
        }

        const auto choice = urand(sum);

        sum = 0;
        size_t i = 0;
        for (const auto &m : moves.moves) {
            sum += values[i];
            ++i;
            if (sum >= choice) {
                s.perform(m);
                return;
            }
        }
        ERROR("unreached");
    }
};
