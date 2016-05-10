#pragma once

#include "agent.h"

#include <thread>
#include <map>

struct MCAgent : Agent, public std::enable_shared_from_this<MCAgent> {
    static constexpr size_t MC_LEN = 70;

    std::string name() const override { return "MCPlayer"; }

    const size_t mc_len;
    const size_t concurrency;

    struct MoveStat {
        int scores;
        int visits;
    };

    using MoveStats = std::map<size_t, MoveStat>;
    mutable std::mutex mtx;

    MCAgent(size_t l=MC_LEN)
    : mc_len(l)
    , concurrency(1) //std::thread::hardware_concurrency())
    {
    }

    size_t randomMove(const Moves &m, State &s) const {
        // Choose a move and perform it.
        size_t i = urand(m.moves.size());
        s.perform(&m.moves[i]);
        if (s.challenge.round.finished()) {
            s.challenge.round.reset();
        }
        return i;
    }

    void finishGame(State &s) const {
        // Play remainder of challenges
        while (!s.gameOver()) {
            Moves m(s);
            randomMove(m, s);
        }
    }

    void searchOne(const Moves &m, MoveStats &stats) const
    {
        // Clone with all random players.
        auto clone = m.state;
        clone.randomizeHiddenState();
        auto p = clone.current().id;
        auto i = randomMove(m, clone);

        finishGame(clone);
        updateStats(stats, clone, p, i);
    }


    void updateStats(MoveStats &stats, const State &s, size_t p, size_t index) const {
        // Find winner, ties considered losses
        auto best = p;
        int high = s.players[p].score;
        for (int j=0; j < s.players.size(); ++j) {
            if (j != p) {
                if (s.players[j].score >= high) {
                    best = j;
                    high = s.players[j].score;
                }
            }
        }

        // Associate win rate with this move
        {
            std::lock_guard<std::mutex> lock(mtx);
            auto &st = stats[index];
            st.scores += s.players[p].score;
            ++st.visits;
        }
    }

    size_t findBest(MoveStats &stats) const {
        int high = 0;
        size_t best = 0;

        size_t index;
        MoveStat s;
        for (auto p : stats) {
            std::tie(index, s) = p;
            
            int avg = s.scores/(float)s.visits;

            // Use >= so we skip over concede if it ties with something else
            if (avg >= high) {
                high = avg;
                best = index;
            }
        }
        return best;
    }

    void move(State &s) const override
    {
        Moves m(s);

        // If only one move, take that one without searching.
        if (m.moves.size() == 1) {
            s.perform(&m.moves[0]);
            return;
        }

        MoveStats stats;

        size_t samples = mc_len * m.moves.size() / concurrency;
        const auto self = shared_from_this();
        std::vector<std::thread> t(concurrency);
        for (size_t i=0; i < concurrency; ++i) {
            t[i] = std::thread([self, &m, &stats, samples] {
                                   for (int j=0; j < samples; ++j) {
                                       self->searchOne(m, stats);
                                   }
                               });
        }
        for (size_t i=0; i < concurrency; ++i) {
            t[i].join();
        }

        s.perform(&m.moves[findBest(stats)]);
    }
};
