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

    int stateValue(const State &s) const {
        const auto &p = s.current();
        int value = p.visible.played_value;
        for (const auto &o : s.players) {
            if (o.id != p.id && !s.challenge.round.conceded.test(o.id)) {
                value -= o.visible.played_value;
            }
        }
        return value;
    }

    int stepValue(const Move::Step &step, const State &s) const {
        if (step.card != CardRef(-1)) {
            return cardValue(Card::Get(step.card), s);
        }
        return 0;
    }

    int stepPairValue(const Move &m, const State &s) const {
        return stepValue(m.first, s) + stepValue(m.second, s);
    }

    int moveValue(const Move &m, const State &s) const {
        if (s.current().discard_two) {
            return -stepPairValue(m, s);
        }
        auto clone = s;
        clone.perform(m);
        return stateValue(clone) - stateValue(s);
    }

    int standardValues(const State &s, const Moves &m, std::vector<int> &values) const {
        int min = 0;
        for (const auto &move : m.moves) {
            auto v = moveValue(move, s);
            values.push_back(v);
            if (v < min) {
                min = v;
            }
        }
        return min;
    }

    int cardMultiplier(Affinity a, const Move &m) const {
        if (a == Affinity::NONE) {
            return 1;
        }

        assert(m.card() != Move::null);
        const auto &card = Card::Get(m.card());
        if (a == card.affinity) {
            return 10;
        } else if (card.affinity == Affinity::NONE) {
            return 5;
        }
        return 1;
    }

    int firstValues(const State &s, const Moves &m, Affinity a, std::vector<int> &values) const {
        int min = 0;
        for (const auto &move : m.moves) {
            auto v = cardMultiplier(a, move)*moveValue(move, s);
            values.push_back(v);
            if (v < min) {
                min = v;
            }
        }
        return min;
    }

    bool isFirstMove(const State &s) const {
        return s.current().visible.empty();
    }

    int normalize(std::vector<int> &values, int min) const {
        int sum = 0;
        for (size_t i=0; i < values.size(); ++i) {
            values[i] = 1+(values[i]-min);
            sum += values[i];
        }
        return sum;
    }

    void move(State &s) {
        const Moves moves(s);

        std::vector<int> values;
        values.reserve(moves.moves.size());

        int min = 0;
        bool found = false;
        if (isFirstMove(s) && !s.current().discard_two) {
            auto best = bestSide(s);
            if (best != Affinity::NONE) {
                found = true;
                min = firstValues(s, moves, best, values);
            }
        }

        if (!found) {
            min = standardValues(s, moves, values);
        }

        auto sum = normalize(values, min);
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
