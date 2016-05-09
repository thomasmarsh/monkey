#pragma once

#include "agent.h"

struct MCPlayer : Player, enable_shared_from_this<MCPlayer> {
    string name() const override { return "MCPlayer"; }
    const size_t mcLen;
    const size_t concurrency;

    struct MoveStat {
        int scores;
        int visits;
    };

    using MoveStats = map<size_t, MoveStat>;
    mutable mutex mtx;

    MCPlayer(size_t l)
    : mcLen(l)
    , concurrency(thread::hardware_concurrency())
    {
    }

    void searchOne(MoveCount &mc, MoveStats &stats) const
    {
        // Clone with all random players.
        auto g2 = mc.state->clone<RandomPlayer>();
        g2->randomizeHiddenState(id);

        // Choose a move and perform it.
        auto index = urand(mc.count);
        auto m = mc.move(index);
        g2->doMove(id, m);

        g2->round.checkState(id, m);

        // Play remainder of challenges
        g2->finishGame();

        updateStats(stats, g2, index);
    }


    void updateStats(MoveStats &stats, GameState::Ptr g2, size_t index) const {
        // Find winner, ties considered losses
        int best = id;
        auto p = g2->players[id];
        int high = p->score;
        for (int j=0; j < g2->players.size(); ++j) {
            if (j != id) {
                if (g2->players[j]->score >= high) {
                    best = j;
                    high = g2->players[j]->score;
                }
            }
        }

        // Associate win rate with this move
        {
            lock_guard<mutex> lock(mtx);
            auto &s = stats[index];
            s.scores += p->score;
            ++s.visits;
        }
    }

    size_t findBest(MoveStats &stats) const {
        int highScore = 0;
        size_t bestScore = 0;

        size_t index;
        MoveStat s;
        for (auto p : stats) {
            tie(index, s) = p;
            
            int avgScore = s.scores/(float)s.visits;

            // Use >= so we skip over concede if it ties with something else
            if (avgScore >= highScore) {
                highScore = avgScore;
                bestScore = index;
            }
        }
        return bestScore;
    }

    Move move(GameState::Ptr gs) override
    {
        LogLock lock;

        auto mc = MoveCount(gs.get());
        if (mc.count == 0) {
            return Move::NullMove();
        }

        // If only one move, take that one without searching.
        if (mc.count == 1) {
            return mc.move(0);
        }
        MoveStats stats;

        size_t samples = mcLen*mc.count / concurrency;
        const auto self = shared_from_this();
        vector<thread> t(concurrency);
        for (size_t i=0; i < concurrency; ++i) {
            t[i] = thread([self,
                           mc,
                           &stats,
                           samples]
            {
                auto mcCopy = mc;
                for (int j=0; j < samples; ++j) {
                    self->searchOne(mcCopy, stats);
                }
            });
        }
        for (size_t i=0; i < concurrency; ++i) {
            t[i].join();
        }

        return mc.move(findBest(stats));
    }
};
