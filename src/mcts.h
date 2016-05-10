#pragma once

#include "agent.h"
#include "naive.h"
#include "mcts_node.h"

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

struct MCTSAgent {
    using MoveList = Node::MoveList;

    size_t itermax;
    size_t num_trees;
    float exploration;
    size_t move_count;
    MCTSRand policy;

    // suggest imax=1,000..10,000, n=8..10, c=0.7
    MCTSAgent(size_t imax=1000, size_t n=8, float c=0.7, MCTSRand p=MCTSRand::ALWAYS)
    : itermax(imax)
    , num_trees(n)
    , exploration(c)
    , move_count(0)
    , policy(p)
    {}

    std::string name() const {
        return fmt::format("MCTS:{}/{}/{}/{}",
                           num_trees,
                           itermax,
                           to_string(policy),
                           exploration);
    }

    // Actions to take before performing a move to make sure we're in the right state.
    void preMove(State &state) {
        // TODO: reevaluate
        if (state.challenge.finished()) {
            state.challenge.reset();
        }
        assert(!state.gameOver());
    }

    // Actions to take after performing a move to make sure we're in the right state.
    void postMove(State &state) {
        // TODO: reevaluate
        if (state.challenge.finished()) {
            state.challenge.reset();
        }
    }

    void makeMove(const Move &move, State &state) {
        preMove(state);
        state.perform(&move);
        postMove(state);
    }

    void log(Node::Ptr root, const State &state) {
        //LOG("exploration tree:");
        //root->printTree();
        //auto p = state->currentPlayer();
        //LOG("player {} children:", p->id);
        //root->printChildren();
        writeDot(root);
    }

    void loop(Node::Ptr root, const State &root_state) {

        auto initial = root_state;
        initial.randomizeHiddenState();

        for (int i=0; i < itermax; ++i) {
            initial = iterate(root, initial, i);
        }
    }

    void move(State &root_state) {
        assert(!root_state.gameOver());
        //return parallelSearch(root_state);
        singleSearch(root_state);
    }

    std::vector<std::pair<Move,size_t>> iterateAndMerge(const State &root_state) {
        ScopedLogLevel lock(LogContext::Level::warn);

        std::vector<Node::Ptr> root_nodes(num_trees);
        std::vector<std::thread> t(num_trees);

        for (size_t i=0; i < num_trees; ++i) {
            root_nodes[i] = Node::New(Move::Null(), Cards{0}, nullptr, -1);
            t[i] = std::thread([this, root_state, root=root_nodes[i]] {
                this->loop(root, root_state);
            });
        }

        std::vector<std::pair<Move,size_t>> merge;
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

    Move parallelSearch(const State &root_state) {
        auto merge = iterateAndMerge(root_state);

        // This can happen at the last move; not entirely sure why.
        if (merge.empty()) {
            return Move::Pass();
        }

        auto best = Move::Null();
        int high = 0;
        LOG("player {} children:", root_state.current().id);
        Sort(merge, [](const auto &a, const auto &b) {
            return a.second > b.second;
        });
        for (const auto &e : merge) {
            LOG(" - {} {}", e.second, to_string(e.first, root_state.current()));
            if (e.second > high) {
                high = e.second;
                best = e.first;
            }
        }

        //LOG("best: {} {}", high, CARD_TABLE[Player::cardForMove(best)].repr().c_str());
        assert(best.isNull());
        return best;
    }

    Move singleSearch(const State &root_state) {
        auto root = Node::New(Move::Null(), Cards{0}, nullptr, -1);

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
    std::pair<State,State> determinize(State rootstate, size_t i) {
        auto initial = rootstate;
        auto state = rootstate;
        if (policy == MCTSRand::ALWAYS || (policy == MCTSRand::ONCE && i == 0)) {
            state.randomizeHiddenState();
            initial = state;
        }
        return {initial, state};
    }

    std::pair<State, Node::Ptr> select(State &state,
                                       Node::Ptr node)
    {
        Moves m(state);
        // TODO: get rid of this copy
        auto moves = m.moves;
        auto untried = node->getUntriedMoves(moves);

        while (!moves.empty())
        {
            if (!untried.empty()) {
                return expand(state, node, untried);
            }
            node = node->selectChildUCB(state, moves, exploration);
            makeMove(node->move, state);

            Moves m2(state);
            // TODO: another copy here
            moves = m2.moves;
            untried = node->getUntriedMoves(moves);
        }
        return {state, node};
    }

    // Create a new node to explore.
    std::pair<State, Node::Ptr> expand(State &state,
                                       Node::Ptr node,
                                       MoveList &untried)
    {
        if (!untried.empty()) {
            auto m = untried[urand(untried.size())];
            auto &p = state.current();
            makeMove(m, state);
            node = node->addChild(m, {m.card}, p.id);
        }
        return {state, node};
    }

    State iterate(Node::Ptr root, State &initial, int i) {
        auto node = root;

        // Determinize
        auto state = initial;
        std::tie(initial, state) = determinize(initial, i);

        // Find next node
        std::tie(state, node) = select(state, node);

        // Simulate
        auto agent = NaiveAgent();
        Rollout(state, agent);

        // Backpropagate
        while (node) {
            node->update(state);
            node = node->parent.lock();
        }
        return initial;
    }

    void writeNodeDefinitions(FILE *fp, Node::Ptr node, int &n, int indent=1) {
        int m = n;
        fprintf(fp, "%*cn%d [\n", indent*2, ' ', m);
        fprintf(fp, "%*c  label = \"%s | %s\"\n",
                indent*2, ' ', 
                node->shortRepr().c_str(),
                EscapeQuotes(to_string(node->move)).c_str());
        fprintf(fp, "%*c  shape = record\n", indent*2, ' ');
        fprintf(fp, "%*c  style = filled\n", indent*2, ' ');
        std::string color;
        switch (node->just_moved) {
        case 0: color = "fc1e33"; break;
        case 1: color = "ff911e"; break;
        case 2: color = "21abd8"; break;
        case 3: color = "40ee1c"; break;
        }
        if (!color.empty()) {
            fprintf(fp, "%*c  fillcolor = \"#%s\"\n", indent*2, ' ', color.c_str());
        }
        fprintf(fp, "%*c];\n", indent*2, ' ');

        for (auto &child : node->children) {
            ++n;
            writeNodeDefinitions(fp, child, n, indent+1);
        }
    }

    void writeNodeConnections(FILE *fp, Node::Ptr node, int &n) {
        int m = n;
        for (auto &child : node->children) {
            ++n;
            fprintf(fp, "  n%d -> n%d;\n", m, n);
            writeNodeConnections(fp, child, n);
        }
    }

    void writeDot(Node::Ptr root) {
        root->sort();

        std::string fname("ismcts/ismcts.");
        fname += std::to_string(move_count);
        FILE *fp = fopen(fname.c_str(), "w");
        assert(fp);

        fprintf(fp, "digraph G {\n");
        fprintf(fp, "  rankdir = LR;\n");

        int n = 0;
        writeNodeDefinitions(fp, root, n);
        n = 0;
        writeNodeConnections(fp, root, n);
        fprintf(fp, "}\n");
        fclose(fp);
    }
};
