#pragma once

#include "core.h"
#include "log.h"

#include <map>
#include <vector>

using CardRef = uint16_t;

constexpr CardRef NUM_CARDS = 198;

struct Card {
    CardRef  id;
    CardRef  prototype;
    uint8_t  quantity;
    CardType type;
    Affinity affinity;
    Special  special;
    Action   action;
    ArgType  arg_type;
    int      face_value;
    int      inverted_value;

    std::string label;

    static const Card& Get(CardRef card);
};

extern std::string ActionDescription(Action a);

inline std::string to_string(const Card &card) {
    return fmt::format("[{:02X} {}{}{} \"{}\"]",
                       card.id,
                       to_string_short(card.type),
                       card.face_value,
                       to_string_short(card.affinity),
                       card.label);
}


void LoadCards();

// ---------------------------------------------------------------------------

using Cards = std::vector<CardRef>;

// Copies all of cards from source to end of target.
inline void CopyCards(const Cards &source, Cards &target) {
    target.insert(end(target), begin(source), end(source));
}

// Moves all of cards from source to end of target.
inline void MoveCards(Cards &source, Cards &target) {
    CopyCards(source, target);
    source.clear();
}

inline CardRef DrawCard(Cards &set) {
    auto card = set.back();
    set.pop_back();
    return card;
}

inline CardRef DrawCard(Cards &set, size_t i) {
    assert(i < set.size());
    auto it = begin(set)+i;
    auto card = *it;
    set.erase(it);
    return card;
};
