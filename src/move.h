#pragma once 

#include "core.h"
#include "cards.h"

#include <memory>

struct Move {
    static constexpr auto null = uint8_t(-1);

    struct Step {
        Action   action;
        CardRef  card;
        uint8_t  index;
        uint8_t  arg;

        Step(Action  a = Action::NONE,
             CardRef c = CardRef(-1),
             uint8_t i = null,
             uint8_t g = null)
        : action(a)
        , card(c)
        , index(i)
        , arg(g)
        {}

        bool operator==(const Step &rhs) const {
            return action == rhs.action &&
                   index  == rhs.index  &&
                   arg    == rhs.arg;
        }

        bool cardEquals(const Step &rhs) const {
            return action == rhs.action &&
                   card   == rhs.card   &&
                   arg    == rhs.arg;
        }

        bool isBasic(Action a) const {
            return action == a && index == null;
        }

        bool isNull()    const { return isBasic(Action::NONE); }
        bool isConcede() const { return isBasic(Action::CONCEDE); }
        bool isPass()    const { return isBasic(Action::PASS); }
    };

    Step first;
    Step second;

    Action  action() const { return first.action; }
    CardRef card()   const { return first.card; }
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

    bool cardEquals(const Move &rhs) const {
        return first.cardEquals(rhs.first) && second.cardEquals(rhs.second);
    }
} __attribute__ ((__packed__));

inline std::string to_string(const Move::Step &s) {
    auto cardStr   = s.card  == CardRef(-1) ? std::string() : to_string(Card::Get(s.card));
    auto indexStr  = s.index == Move::null  ? std::string() : std::to_string(s.index);
    auto argStr    = s.arg   == Move::null  ? std::string() : std::to_string(s.arg);

    auto str = "{ " + to_string(s.action);
    for (const auto &part : { cardStr, indexStr, argStr }) {
        if (!part.empty()) {
            str += " " + part;
        }
    }
    str += " }";
    return str;
}

inline std::string to_string(const Move &m) {
    if (m.second.isNull()) {
        return to_string(m.first);
    }
    return fmt::format("{{ {} {} }}",
                       to_string(m.first),
                       to_string(m.second));
}
