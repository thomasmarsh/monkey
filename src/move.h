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

    bool operator==(const Move &rhs) const {
        return action == rhs.action &&
               card   == rhs.card   &&
               arg    == rhs.arg    &&
               *next  == *rhs.next;
    }
} __attribute__ ((__packed__));

inline std::string to_string(const Move &m) {
    auto s = fmt::format("{{ {} {{ {} }} {} }}",
                         to_string(m.action),
                         m.card == Move::null
                             ? std::string("null")
                             : std::to_string(m.card),
                         m.arg == Move::null
                             ? std::string("null")
                             : std::to_string(m.arg));
    auto *move = m.next;
    while (move) {
        s += " " + to_string(*move);
        move = move->next;
    }
    return s;
}
