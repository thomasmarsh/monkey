#pragma once 

#include "core.h"

#include <memory>

struct Move {
    using Ptr = Move*;

    Action   action;
    uint8_t  card;
    uint8_t  arg;
    Ptr      next;

    static constexpr auto null = uint8_t(-1);

    static Move Null()    { return Move {Action::NONE,    null, null, nullptr}; }
    static Move Concede() { return Move {Action::CONCEDE, null, null, nullptr}; }
    static Move Pass()    { return Move {Action::PASS,    null, null, nullptr}; }

    bool isNull() const {
        return action == Action::NONE && card == null;
    }
} __attribute__ ((__packed__));
