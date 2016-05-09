#pragma once 

#include "core.h"

#include <memory>

struct Move {
    Action   action;
    size_t   card;
    size_t   arg;
    std::shared_ptr<Move> next;

    static constexpr auto null = size_t(-1);

    Move(Action a, size_t c, size_t g, const std::shared_ptr<Move> n)
    : action(a)
    , card(c)
    , arg(g)
    , next(n)
    {}

    static Move Null()    { return Move(Action::NONE, null, null, nullptr); }
    static Move Concede() { return Move(Action::CONCEDE, null, null, nullptr); }
    static Move Pass()    { return Move(Action::PASS, null, null, nullptr); }

    bool isNull() const {
        return action == Action::NONE && card == null;
    }
};
