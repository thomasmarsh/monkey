#pragma once

#include "state.h"
#include "moves.h"


struct NaiveAgent {
    using Values = std::vector<int>;
    bool quiet;

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

    int stateValue(const State &s, const uint8_t p) const {
        int value = s.players[p].visible.played_value;
        for (const auto &o : s.players) {
            if (o.id != p && !s.challenge.round.conceded.test(o.id)) {
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
        const auto a = stateValue(s, s.current().id);
        const auto b = stateValue(clone, s.current().id);
        return b - a;
    }

    int standardValues(const State &s, const Moves &m, Values &values) const {
        int min = 0;
        auto best = bestSide(s);
        for (const auto &move : m.moves) {
            auto v = cardAdjust(best, move) + moveValue(move, s);
            values.push_back(v);
            if (v < min) {
                min = v;
            }
        }
        return min;
    }

    int cardAdjust(Affinity target, const Move &m) const {
        if (target == Affinity::NONE || m.card() == Move::null) {
            return 0;
        }
        const auto &card = Card::Get(m.card());
        if (target == card.affinity || card.affinity == Affinity::NONE) {
            return 0;
        }
        return -40;
    }

    int firstValues(const State &s, const Moves &m, Affinity target, Values &values) const {
        int min = 0;
        for (const auto &move : m.moves) {
            assert(move.card() != Move::null);
            auto v = cardAdjust(target, move)+moveValue(move, s);
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

    int normalize(Values &values, int min) const {
        int sum = 0;
        for (size_t i=0; i < values.size(); ++i) {
            values[i] = 1+(values[i]-min);
            sum += values[i];
        }
        return sum;
    }

    int checkFirstMove(const State &s, const Moves &moves, Values &values) const {
        int min = 0;
        if (isFirstMove(s) && !s.current().discard_two) {
            auto best = bestSide(s);
            if (best != Affinity::NONE) {
                min = firstValues(s, moves, best, values);
            }
        }
        return min;
    }

    void print(const Moves &moves, const Values &values) {
        if (!quiet) {
            printf("VALUE:\n");
            for (int i=0; i < values.size(); ++i) {
                printf("    %3d: %s\n", values[i], to_string(moves.moves[i]).c_str());
            }
        }
    }

    int findMoveValues(const State &s, const Moves &moves, Values &values) const {
        auto min = checkFirstMove(s, moves, values);
        if (values.empty()) {
            min = standardValues(s, moves, values);
        }
        return min;
    }

    void perform(State &s, const size_t choice, const Moves &moves, const Values &values) const {
        int sum = 0;
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

    void move(State &s) {
        quiet = s.quiet;
        const Moves moves(s);

        std::vector<int> values;
        values.reserve(moves.moves.size());

        const auto min = findMoveValues(s, moves, values);
        const auto sum = normalize(values, min);
        //print(moves, values);
        perform(s, urand(sum), moves, values);
    }
};
