#pragma once 

#include "cards.h"

struct PlayerHand {
    std::vector<CardRef> characters;
    std::vector<CardRef> skills;

    size_t size() const { return characters.size() + skills.size(); }
    bool empty() const { return characters.empty() && skills.empty(); }

    void reset() {
        characters.clear();
        skills.clear();
    }

    void insert(CardRef c) {
        assert(c < NUM_CARDS);
        if (Card::Get(c).type == CHARACTER) {
            characters.push_back(c);
        } else {
            skills.push_back(c);
        }
    }

    CardRef at(size_t i) const {
        if (i >= characters.size()) {
            return skills[i-characters.size()];
        }
        return characters[i];
    }

    CardRef draw(size_t i) {
        if (i >= characters.size()) {
            assert(i-characters.size() < skills.size());
            return DrawCard(skills, i-characters.size());
        }
        return DrawCard(characters, i);
    }

    CardRef drawRandom() {
        return draw(urand(size()));
    }

    void debug() const {
        DLOG("Hand:");
        for (auto c : characters) {
            DLOG("    {} {}", c, to_string(Card::Get(c)));
        }
        for (auto c : skills) {
            DLOG("    {} {}", c, to_string(Card::Get(c)));
        }
    }
};
