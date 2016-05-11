#pragma once 

#include "core.h"

#include <memory>

struct Move {
    static constexpr auto null = uint8_t(-1);

    struct Step {
        Action   action;
        uint8_t  index;
        uint8_t  arg;

        Step(Action a=Action::NONE, uint8_t i=null, uint8_t g=null)
        : action(a), index(i), arg(g) {}

        bool operator==(const Step &rhs) const {
            return action == rhs.action &&
                   index  == rhs.index   &&
                   arg    == rhs.arg;
        }

        bool isCore(Action a) const {
            return action == a && index == null;
        }
        bool isNull() const { return isCore(Action::NONE); }
        bool isConcede() const { return isCore(Action::CONCEDE); }
        bool isPass() const { return isCore(Action::PASS); }
    };

    Step first;
    Step second;

    Action  action() const { return first.action; }
    uint8_t index()  const { return first.index; }
    uint8_t arg()    const { return first.arg; }

    static Move Null()    { return {Step(),                Step()}; };
    static Move Concede() { return {Step(Action::CONCEDE), Step()}; }
    static Move Pass()    { return {Step(Action::PASS),    Step()}; }

    bool isNull()    const { return first.isNull(); }
    bool isConcede() const { return first.isConcede(); }
    bool isPass()    const { return first.isPass(); }

    bool operator==(const Move &rhs) const {
        return first == rhs.first && second == rhs.second;
    }

} __attribute__ ((__packed__));

inline std::string to_string(const Move::Step &s) {
    return fmt::format("{{ {} {{ {} }} {} }}",
                       to_string(s.action),
                       s.index == Move::null
                           ? std::string("null")
                           : std::to_string(s.index),
                       s.arg == Move::null
                           ? std::string("null")
                           : std::to_string(s.arg));
};

inline std::string to_string(const Move &m) {
    if (m.second.isNull()) {
        return to_string(m.first);
    }
    return fmt::format("{{ {} {} }}",
                       to_string(m.first),
                       to_string(m.second));
}
