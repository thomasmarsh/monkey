#pragma once

#include "agent.h"

#include <thread>
#include <map>

struct MCAgent {
    static constexpr size_t MC_LEN = 70;

    std::string name() const { return "MCPlayer"; }

    const size_t mc_len;
    const size_t concurrency;

    struct MoveStat {
        int scores;
        int visits;

        float average() { return float(scores) / float(visits); }
    };

    using MoveStats = std::map<size_t, MoveStat>;
    mutable std::mutex mtx;

    MCAgent(size_t l=MC_LEN)
    : mc_len(l)
    , concurrency(std::thread::hardware_concurrency())
    {
        TRACE();
    }

    void searchOne(const Moves &m, MoveStats &stats) const {
        TRACE();
        // Clone and randomize
        auto clone = m.state;
        clone.randomizeHiddenState();
        assert(clone.deck.get() != m.state.deck.get());
        assert(*clone.deck != *m.state.deck);

        //  Remember which player was about to play
        auto p = clone.current().id;

        // Select a move and perform it
        ForwardState(clone);
        auto i = RandomMove(m, clone);
        ForwardState(clone);

        // Randomly rollout the remainder
        auto agent = RandomAgent();
        Rollout(clone, agent);

        updateStats(stats, clone, p, i);
    }

    void updateStats(MoveStats &stats, const State &s, size_t p, size_t index) const {
        TRACE();
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
        TRACE();
        int high = 0;
        size_t best = 0;

        size_t index;
        MoveStat s;
        for (auto p : stats) {
            std::tie(index, s) = p;
            
            float avg = (float)s.scores/(float)s.visits;

            // Use >= so we skip over concede if it ties with something else
            if (avg >= high) {
                high = avg;
                best = index;
            }
        }
        return best;
    }

    void search(const Moves &m, MoveStats &stats, size_t samples) {
        TRACE();
        for (int i=0; i < samples; ++i) {
            searchOne(m, stats);
        }
    }

    void dispatchThreads(const Moves &m, MoveStats &stats, size_t samples) {
        std::vector<std::thread> t(concurrency);
        for (size_t i=0; i < concurrency; ++i) {
            t[i] = std::thread([this, &m, &stats, samples] {
                this->search(m, stats, samples);
            });
        }
        for (size_t i=0; i < concurrency; ++i) {
            t[i].join();
        }
    }

    void dispatchSearch(const Moves &m, MoveStats &stats) {
        ScopedLogLevel l(LogContext::Level::warn);
        size_t samples = mc_len * m.moves.size() / concurrency;
        WARN("samples = {} ({} * {} / {})", samples, mc_len, m.moves.size(), concurrency);
        if (concurrency == 1) {
            search(m, stats, samples);
        } else {
            dispatchThreads(m, stats, samples);
        }
    }

    void move(State &s) {
        TRACE();

        Moves m(s);
        assert(!m.moves.empty());

        // If only one move, take that one without searching.
        if (m.moves.size() == 1) {
            s.perform(m.moves[0]);
            return;
        }

        MoveStats stats;
        dispatchSearch(m, stats);
        logStats(m, stats);
        s.perform(m.moves[findBest(stats)]);
    }

    using SortedStats = std::vector<std::pair<size_t,MoveStat>>;

    SortedStats sortStats(const MoveStats &stats) const {
        SortedStats sorted;

        std::copy(stats.begin(), stats.end(), std::back_inserter(sorted));

        std::sort(sorted.begin(), sorted.end(), [](auto &a, auto &b) {
            return a.second.average() > b.second.average();
        });

        return sorted;
    }

    void logStats(const Moves &m, const MoveStats &stats) const {
        LOG("Search results:");

        size_t index;
        MoveStat stat;
        for (const auto &entry : sortStats(stats)) {
            std::tie(index, stat) = entry;
            LOG("    {:3.2f} {}", stat.average(), to_string(m.moves[index], m.state.current()));
        }
    }

};
