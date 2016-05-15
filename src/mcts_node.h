#pragma once

#include "util.h"

// TODO: generalize Node to Node<S, M>

template <typename M, typename S, typename Contains>
struct Node : std::enable_shared_from_this<Node<M,S,Contains>> {
    using Ptr = std::shared_ptr<Node>;
    using wPtr = std::weak_ptr<Node>;

    using MoveList = std::vector<M>;

    M move;
    Cards cards;
    // The move that got us to this node. null for the root node.
    wPtr parent;
    std::vector<Ptr> children;
    float wins;
    size_t visits;
    size_t avails;
    int just_moved;

    Node(const M &m, Cards c, Ptr _parent, int p)
    : move(m)
    , cards(c)
    , parent(_parent)
    , wins(0)
    , visits(0)
    , avails(1)
    , just_moved(p)
    {
    }

    static Ptr New(const M &m, const Cards &c, const Ptr &parent, int p) {
        return std::make_shared<Node>(m, c, parent, p);
    }

    void validate(const Move::Step &step) {
#ifndef NDEBUG
        if (step.card != CardRef(-1)) {
            auto card = Card::Get(step.card);
            if (card.type == STYLE) {
                assert(step.index != Move::null);
            }
        }
#endif
    }

    void validate(const Move &move) {
        validate(move.first);
        validate(move.second);
    }

    void reset(const M &m, const Cards &c, const Ptr &_parent, int p) {
        TRACE();
        move   = m;
        cards  = c;
        parent = _parent;
        wins   = 0;
        visits = 0;
        avails = 1;
        just_moved = p;
    }

    Ptr self() { return this->shared_from_this(); }

    // Returns the elements of the legal_moves for which this node does not have children.
    MoveList getUntriedMoves(MoveList legal_moves) {
        TRACE();
        if (legal_moves.empty()) {
            return {};
        }

        // Find all moves for which this node *does* have children.
        MoveList tried;
        DLOG("children.size() = {}", children.size());
        for (const auto &c : children) {
            if (!c->move.isNull()) {
                validate(c->move);
                tried.emplace_back(c->move);
            }
        }

        MoveList untried;
        for (const auto m : legal_moves) {
            if (Contains().find(tried, m).isNull()) {
                validate(m);
                TLOG("add untried: {}", to_string(m));
                untried.emplace_back(m);
            }
        }
        return untried;
    }

    float ucb(float exploration) {
        constexpr float EPSILON = 1e-6;
        auto tiebreaker = RandFloat() * EPSILON;
        return (float) wins / (float) visits
            + exploration * sqrt(log(avails) / (float) visits)
            + tiebreaker;
    }

    // Use the UCB1 formula to select a child node, filtered by the given list
    // of legal moves.
    Ptr selectChildUCB(std::shared_ptr<S> &state,
                       const MoveList &legal_moves,
                       float exploration) {
        TRACE();
        // Filter the list of children by the list of legal moves.
        std::vector<Ptr> legal_children;
        for (const auto &child : children) {
            auto move = Contains().find(legal_moves, child->move);
            if (!move.isNull()) {
                // Update child move with legal context
                child->move = move;

                validate(child->move);
                legal_children.push_back(child);
            }
        }

        // Get the child with the highest UCB score
        auto s = std::max_element(begin(legal_children),
                                  end(legal_children),
                                  [exploration](const auto &a, const auto &b) {
                                    return a->ucb(exploration) < b->ucb(exploration);
                                  });

        // Update availability counts -- it is easier to do this now than during
        // backpropagation
        for (auto &child : legal_children) {
            child->avails += 1;
        }

        // Return the child selected above
        return *s;
    }

    // Add a new child for the move m, returning the added child node.
    Ptr addChild(M m, Cards c, int p) {
        TRACE();
        auto n = New(m, c, self(), p);
        children.push_back(n);
        return n;
    }

    // Update this node - increment the visit count by one and increase the
    // win count by the result of the terminalState for just_moved
    void update(const std::shared_ptr<S> &terminal) {
        TRACE();
        visits += 1;
        if (just_moved != -1) {
            wins += terminal->getResult(just_moved);
        }
    }

    std::string repr(const std::string &move_repr) const {
        return fmt::format("[w/v/a: {:4}/{:4}/{:4} p={} m={}",
                           wins, visits, avails, just_moved, move_repr);
    }

    std::string shortRepr() const {
        return fmt::format("{}/{}/{}", wins, visits, avails);
    }

    void printChildren() const {
        for (const auto &n : children) {
            LOG(" - {}", n->repr(to_string(n->move)));
        }
    }

    void printTree(size_t indent=0) const {
        DLOG("{}", indentStr(indent) + repr(to_string(move)));

        for (const auto &c : children) {
            c->printTree(indent+1);
        }
    }

    std::string indentStr(size_t indent) const {
        TRACE();
        auto s = std::string();
        for (int i=0; i < indent; ++i) {
            s += "| ";
        }
        return s;
    }

    void sort() {
        TRACE();
        Sort(children, [](const auto &a, const auto &b) {
            return a->visits > b->visits;
        });

        for (auto &child : children) {
            child->sort();
        }
    }
};
