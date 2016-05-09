#pragma once

#include "agent.h"
#include "mcts_node.h"

// Randomization policy for ISMCTSPlayer
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

struct ISMCTSPlayer : Player {
    size_t itermax;
    size_t num_trees;
    float exploration;
    size_t move_count;
    MCTSRand policy;

    using NodeT = Node<Move>::Ptr;

    // suggest imax=1,000..10,000, n=8..10, c=0.7
    ISMCTSPlayer(size_t imax, size_t n, float c, MCTSRand p)
    : itermax(imax)
    , num_trees(n)
    , exploration(c)
    , move_count(0)
    , policy(p)
    {}

    string name() const override {
        return "ISMCTS:" + to_string(num_trees) + "/"
                         + to_string(itermax)   + "/"
                         + PolicyStr(policy)    + "/"
                         + to_string(exploration);
    }

    // Actions to take before performing a move to make sure we're in the right state.
    void preMove(const GameState::Ptr &state) {
        if (state->round.challenge_finished) {
            state->beginChallenge();
        } else if (state->round.round_finished) {
            state->round.reset();
        }
        assert(!state->game_over);
    }

    // Actions to take after performing a move to make sure we're in the right state.
    void postMove(const GameState::Ptr &state) {
        if (state->round.challenge_finished) {
            state->endChallenge();
            if (!state->gameOver()) {
                state->beginChallenge();
            }
        } else if (state->round.round_finished) {
            if (!state->game_over) {
                state->round.reset();
            }
        }
    }

    void makeMove(Move move, const GameState::Ptr &state) {
        preMove(state);

        auto p = state->currentPlayer()->id;
        state->doMove(p, move);
        state->round.checkState(p, move);

        postMove(state);
    }

    void log(NodeT root, GameState::Ptr state) {
        //LOG("exploration tree:");
        //root->printTree();
        //auto p = state->currentPlayer();
        //LOG("player %zu children:", p->id);
        //root->printChildren();
        writeDot(root);
    }

    void loop(NodeT root, GameState::Ptr root_state) {

        auto initial = root_state->clone<NaivePlayer>();
        initial->randomizeHiddenState(id);

        for (int i=0; i < itermax; ++i) {
            initial = iterate(root, initial, i);
        }
    }

    Move move(GameState::Ptr root_state) override {
        assert(!root_state->game_over);
        return parallelSearch(root_state);
        //return singleSearch(root_state);
    }

    vector<pair<Move,size_t>> iterateAndMerge(GameState::Ptr root_state) {
        LogLock lock;

        vector<NodeT> root_nodes(num_trees);
        vector<thread> t(num_trees);

        {
            for (size_t i=0; i < num_trees; ++i) {
                root_nodes[i] = make_shared<Node>(Move::NullMove(), Cards{0}, nullptr, -1);
                t[i] = thread([this, root_state, root=root_nodes[i]] {
                    this->loop(root, root_state);
                });
            }
        }

        vector<pair<Move,size_t>> merge;
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

    Move parallelSearch(GameState::Ptr root_state) {
        auto merge = iterateAndMerge(root_state);

        // This can happen at the last move; not entirely sure why.
        if (merge.empty()) {
            return {Move::PASS, 0};
        }

        auto best = Move {Move::NULL_ACTION, 0};
        int high = 0;
        LOG("player %zu children:", id);
        Sort(merge, [](const auto &a, const auto &b) {
            return a.second > b.second;
        });
        for (const auto &e : merge) {
            LOG(" - %lu %s", e.second, reprForMove(e.first).c_str());
            if (e.second > high) {
                high = e.second;
                best = e.first;
            }
        }

        //LOG("best: %lu %s", high, CARD_TABLE[Player::cardForMove(best)].repr().c_str());
        assert(!Move::IsNullMove(best));
        return best;
    }

    Move singleSearch(GameState::Ptr root_state) {
        auto root = make_shared<Node>(Move::NullMove(), Cards{0}, nullptr, -1);

        loop(root, root_state);

        log(root, root_state);

        // This can happen at the last move; not entirely sure why.
        if (root->children.empty()) {
            return {Move::PASS, 0};
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
    pair<GameState::Ptr,GameState::Ptr> determinize(GameState::Ptr rootstate, size_t i) {
        GameState::Ptr initial = rootstate;
        GameState::Ptr state = rootstate->clone<NaivePlayer>();
        if (policy == MCTSRand::ALWAYS || (policy == MCTSRand::ONCE && i == 0)) {
            state->randomizeHiddenState(id);
            initial = state->clone<NaivePlayer>();
        }
        return {initial, state};
    }

    pair<GameState::Ptr, NodeT> select(GameState::Ptr state,
                                       NodeT node)
    {
        auto moves = state->getCurrentMoveList();
        auto untried = node->getUntriedMoves(moves);

        while (!moves.empty())
        {
            if (!untried.empty()) {
                return expand(state, node, untried);
            }
            node = node->selectChildUCB(state, moves, exploration);
            makeMove(node->move, state);

            moves = state->getCurrentMoveList();
            untried = node->getUntriedMoves(moves);
        }
        return {state, node};
    }

    // Create a new node to explore.
    pair<GameState::Ptr, NodeT> expand(GameState::Ptr state,
                                           NodeT node,
                                           MoveList &untried)
    {
        if (!untried.empty()) {
            auto m = untried[urand(untried.size())];
            auto p = state->currentPlayer();
            auto c = cardsForMove(m, p);
            makeMove(m, state);
            node = node->addChild(m, c, p->id);
        }
        return {state, node};
    }

    GameState::Ptr iterate(NodeT root, GameState::Ptr initial, int i) {
        auto node = root;

        // Determinize
        GameState::Ptr state;
        tie(initial,state) = determinize(initial, i);

        // Find next node
        tie(state, node) = select(state, node);

        // Simulate
        state->finishGame();

        // Backpropagate
        while (node) {
            node->update(state);
            node = node->parent.lock();
        }
        return initial;
    }

    Cards cardsForMove(const Move &m, Player *p) const {
        Cards c {0};
        switch (m.action) {
        case Move::PLAY_STYLE:
            c = p->cardsForStyle(m.index);
            break;
        case Move::PLAY_CHARACTER:
        case Move::PLAY_WEAPON:
        case Move::PLAY_WRENCH:
            c[0] = p->cardForMove(m);
            break;
        default:
            break;
        }
        return c;
    }

    void writeNodeDefinitions(FILE *fp, NodeT node, int &n, int indent=1) {
        int m = n;
        fprintf(fp, "%*cn%d [\n", indent*2, ' ', m);
        fprintf(fp, "%*c  label = \"%s | %s\"\n",
                indent*2, ' ', 
                node->shortRepr().c_str(),
                EscapeQuotes(node->smartMoveRepr()).c_str());
        fprintf(fp, "%*c  shape = record\n", indent*2, ' ');
        fprintf(fp, "%*c  style = filled\n", indent*2, ' ');
        string color;
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

    void writeNodeConnections(FILE *fp, NodeT node, int &n) {
        int m = n;
        for (auto &child : node->children) {
            ++n;
            fprintf(fp, "  n%d -> n%d;\n", m, n);
            writeNodeConnections(fp, child, n);
        }
    }

    void writeDot(NodeT root) {
        root->sort();

        string fname("ismcts/ismcts.");
        fname += to_string(move_count);
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
