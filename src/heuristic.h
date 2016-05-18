#pragma once

#include "state.h"
#include "moves.h"

struct HeuristicAgent {
    std::string name() const { return "Heuristic"; }

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

    bool isFirstMove(const State &s) const {
        return s.current().visible.empty();
    }

    void sort(Affinity best, std::vector<Move> &m, const State &s) const {
        std::sort(m.begin(), m.end(), [this, best, &s](const auto &a, const auto &b) {
            assert(a.card() != CardRef(-1));
            assert(b.card() != CardRef(-1));
            const auto &ca = Card::Get(a.card());
            const auto &cb = Card::Get(b.card());
            const auto va = this->cardValue(ca, s);
            const auto vb = this->cardValue(cb, s);

            if (best == Affinity::NONE) {
                // Take smallest
                return va < vb;
            }

            const auto fa = ca.affinity;
            const auto fb = cb.affinity;

            // We have an affinity, but it's irrelevant because the cards are the
            // same affinity.
            if (fa == fb) {
                // Take smallest
                return va < vb;
            }

            auto worst = (best == Affinity::MONK) ? Affinity::CLAN : Affinity::MONK;

            // best = (monk | clan)
            // fa = (best | none | worst)
            // fb = (best | none | worst)
            // fa != fb
            // 
            //      a        b        t/f
            //      ---------------------
            //      best  >  none     t
            //      best  >  worst    t
            //      worst <  best     f
            //      worst <  none     f
            //      none  >  worst    t
            //      none  <  best     f

            if      (fa == best)  { return true; }
            else if (fa == worst) { return false; }
            else if (fb == worst) { return true; }
            else if (fb == best)  { return false; }
            ERROR("unhandled");
        });
    }

    void bestMove(State &s, Moves &m, Affinity a) const {
        sort(a, m.moves, s);
#ifndef NO_LOGGING
        LOG("sorted:");
        for (const auto &move : m.moves) {
            LOG("    {}", to_string(move));
        }
#endif
        s.perform(m.moves[0]);
    }

    bool currentHigh(const State &s) const {
        auto current = s.current().score;
        for (const auto &p : s.players) {
            if (p.id != s.current().id && p.score >= current) {
                return false;
            }
        }
        return true;
    }

    int tryMove(const State &s, const Move &m) const {
        auto clone = s;
        auto id = s.current().id;
        clone.perform(m);
        return clone.players[id].visible.played_value;
    }

    void search(State &s, Moves &m) const {
        // We have the high score, so no move necessary
        if (currentHigh(s)) {
            // TODO: could get rid of low value cards.
            s.perform(Move::Pass());
            return;
        }

        std::vector<std::pair<int,Move>> scores;
        scores.reserve(m.moves.size());
        for (const auto &move : m.moves) {
            scores.push_back({tryMove(s, move), move});
        }
        std::sort(scores.begin(), scores.end(), [](const auto &a, const auto &b) {
            return a.first < b.first;
        });
#ifndef NO_LOGGING
        LOG("sorted:");
        for (const auto &score : scores) {
            LOG("    {} : {}", score.first, to_string(score.second));
        }
#endif
        abort();
    }

    void move(State &s) const {
        Moves moves(s);
        if (isFirstMove(s)) {
            bestMove(s, moves, bestSide(s));
        } else {
            search(s, moves);
        }
    }
};
