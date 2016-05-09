#pragma once 

#include "core.h"

#include <memory>

struct Move {
    Action   action;
    size_t   card;
    size_t   arg;
    std::shared_ptr<Move> next;

    static constexpr auto null = size_t(-1);

    Move(Action a=Action::NONE,
         size_t c=null,
         size_t g=null,
         const std::shared_ptr<Move> n=nullptr)
    : action(a)
    , card(c)
    , arg(g)
    , next(n)
    {}

    static Move Null()    { return {Action::NONE}; }
    static Move Concede() { return {Action::CONCEDE}; }
    static Move Pass()    { return {Action::PASS}; }

    bool isNull() const {
        return action == Action::NONE && card == null;
    }
};
