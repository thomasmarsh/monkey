#pragma once

#include "agent.h"
#include "naive.h"
#include "mcts_node.h"
#include "dot.h"

// Randomization policy for MCTSAgent
enum class MCTSRand {
    NEVER,  // use the same hidden state for every tree
    ONCE,   // use one hidden state per tree
    ALWAYS  // use a different state per simulation
};

static std::string to_string(MCTSRand p) {
    switch (p) {
    case MCTSRand::NEVER:  return "0";
    case MCTSRand::ONCE:   return "1";
    case MCTSRand::ALWAYS: return "*";
    }
};

struct MoveListContains {
    Move find(const std::vector<Move> &moves, const Move &query) const {
        for (const auto &m : moves) {
            if (m.cardEquals(query)) {
                return m;
            }
        }
        return Move::Null();
    }
};

struct MCTSAgent {
    using NodeT = Node<Move, State, MoveListContains>;
    using MoveList = NodeT::MoveList;
    using StatePtr = std::shared_ptr<State>;

    // Used for dot writer
    static size_t move_count;

    const size_t itermax;
    const size_t num_trees;
    const float exploration;
    const MCTSRand policy;

    // suggest imax=1,000..10,000, n=8..10, c=0.7
    MCTSAgent(size_t imax=1000, size_t n=8, float c=0.7, MCTSRand p=MCTSRand::ALWAYS)
    : itermax(imax)
    , num_trees(n)
    , exploration(c)
    , policy(p)
    {}

    std::string name() const {
        return fmt::format("MCTS:{}/{}/{}/{}",
                           num_trees,
                           itermax,
                           to_string(policy),
                           exploration);
    }

    void validate(const Move::Step &step) const {
#ifndef NDEBUG
        if (step.card != CardRef(-1)) {
            auto card = Card::Get(step.card);
            if (card.type == STYLE) {
                assert(step.index != Move::null);
            }
        }
#endif
    }

    void validate(const Move &move) const {
        validate(move.first);
        validate(move.second);
    }

    void perform(const Move &move, StatePtr &state) const {
        TRACE();
        validate(move);
        state->perform(move);
        state->checkReset();
    }

    void loop(NodeT::Ptr root, const State &root_state) const {
        TRACE();
#ifndef NO_LOGGING
        ScopedLogLevel l(LogContext::Level::warn);
#endif

        auto initial = State::New(root_state);
        initial->randomizeHiddenState();

        for (int i=0; i < itermax; ++i) {
            initial = iterate(root, initial, i);
        }
    }

    void move(State &root_state) const {
        TRACE();
        assert(!root_state.gameOver());
        auto m = parallelSearch(root_state);
        //auto m = singleSearch(root_state);
        root_state.perform(m);
    }

    std::vector<std::pair<Move,size_t>> iterateAndMerge(const State &root_state) const {
        TRACE();

        std::vector<NodeT::Ptr> root_nodes(num_trees);
        std::vector<std::thread> t(num_trees);

        for (size_t i=0; i < num_trees; ++i) {
            root_nodes[i] = NodeT::New(Move::Null(), Cards{0}, nullptr, -1);
            t[i] = std::thread([this, i, root_state, root=root_nodes[i]] {
                this->loop(root, root_state);
            });
        }

        std::vector<std::pair<Move,size_t>> merge;
        merge.reserve(num_trees);
        for (size_t i=0; i < num_trees; ++i) {
            t[i].join();
        }
        for (size_t i=0; i < num_trees; ++i) {
            for (const auto &n : root_nodes[i]->children) {
                bool found = false;
                for (int j=0; j < merge.size(); ++j) {
                    if (merge[j].first == n->move) {
                        merge[j].second += 1+n->wins;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    merge.push_back({n->move, 1+n->wins});
                }
            }
        }
        return merge;
    }

    Move parallelSearch(const State &root_state) const {
        TRACE();
#ifndef NO_LOGGING
        ScopedLogLevel l(LogContext::Level::warn);
#endif
        auto merge = iterateAndMerge(root_state);

        // This can happen at the last move; not entirely sure why.
        if (merge.empty()) {
            return Move::Pass();
        }

        auto best = Move::Null();
        unsigned long high = 0;
#ifndef NO_LOGGING
        //ScopedLogLevel l2(LogContext::Level::info);
#endif
        BASE_LOG(info, "Search results:");
        Sort(merge, [](const auto &a, const auto &b) { return a.second > b.second; });
        unsigned long sum = 0;
        for (const auto &e : merge) {
            sum += e.second;
        }
        for (const auto &e : merge) {
            BASE_LOG(info, "    {:-2.2f} {}", 100*float(e.second)/float(sum), to_string(e.first));
            if (e.second > high) {
                high = e.second;
                best = e.first;
            }
        }

        assert(!best.isNull());
        return best;
    }

    Move singleSearch(const State &root_state) const {
        TRACE();
        auto root = NodeT::New(Move::Null(), Cards{0}, nullptr, -1);

        loop(root, root_state);
        log(root, root_state);

        // This can happen at the last move; not entirely sure why.
        if (root->children.empty()) {
            return Move::Pass();
        }

        // Helper for writeDot
        ++move_count;

        // Find the most visited node.
        auto best = std::max_element(begin(root->children),
                                     end(root->children),
                                     [](const auto &a, const auto &b) {
                                         return a->visits < b->visits;
                                     });

        return (*best)->move;
    }

    // Randomize the hidden state.
    std::pair<StatePtr,StatePtr> determinize(const StatePtr &root_state, size_t i) const {
        TRACE();
        auto initial = root_state;
        auto state = State::New(*root_state);
        if (policy == MCTSRand::ALWAYS || (policy == MCTSRand::ONCE && i == 0)) {
            state->randomizeHiddenState();
            initial = State::New(*state);
        }
        return {initial, state};
    }

    std::vector<Move> search(StatePtr state, NodeT::Ptr node) const {
        TRACE();
#ifndef NO_LOGGING
        ScopedLogLevel l(LogContext::Level::warn);
#endif
        Moves m(*state);
        return m.moves;
    }

    std::pair<StatePtr, NodeT::Ptr> select(StatePtr state, NodeT::Ptr node) const {
        TRACE();
        auto moves = search(state, node);
        DLOG("moves.size() = {}", moves.size());

        auto untried = node->getUntriedMoves(moves);

        while (!moves.empty()) {
            if (!untried.empty()) {
                return expand(state, node, untried);
            }
            node = node->selectChildUCB(state, moves, exploration);
            perform(node->move, state);

            moves = search(state, node);
            untried = node->getUntriedMoves(moves);
        }
        return {state, node};
    }

    // Create a new node to explore.
    std::pair<StatePtr, NodeT::Ptr> expand(StatePtr state, NodeT::Ptr node, MoveList &untried) const {
        TRACE();
        if (!untried.empty() && !state->gameOver()) {
            auto m = untried[urand(untried.size())];
            auto &p = state->current();
            perform(m, state);
            node = node->addChild(m, {m.card()}, p.id);
        }
        return {state, node};
    }

    StatePtr iterate(NodeT::Ptr root, StatePtr initial, int i) const {
        TRACE();
        auto node = root;

        // Determinize
        auto state = initial;
        std::tie(initial, state) = determinize(initial, i);

        // Find next node
        std::tie(state, node) = select(state, node);

        // Simulate
        auto agent = NaiveAgent();
        if (!state->gameOver()) {
            Rollout(*state, agent);
        }

        // Backpropagate
        while (node) {
            node->update(state);
            node = node->parent.lock();
        }
        return initial;
    }

    void log(NodeT::Ptr root, const State &state) const {
        DLOG("exploration tree:");
        root->printTree();
        LOG("player {} children:", state.current().id);
        root->sort();
        root->printChildren();
        DotWriter<NodeT>(root, move_count);
    }
};
