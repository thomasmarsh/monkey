#pragma once 

#include "cards.h"

struct PlayerHand {
    size_t               num_characters;
    size_t               num_skills;
    std::vector<CardRef> cards;

    PlayerHand()
    : num_characters(0)
    , num_skills(0)
    {
    }

    size_t size() const { return cards.size(); }
    bool empty() const { return cards.empty(); }

    void reset() {
        cards.clear();
        num_skills = 0;
        num_characters = 0;
    }

    void insert(CardRef c) {
        cards.push_back(c);

        auto &card = Card::Get(c);
        switch (card.type) {
        case CHARACTER:
            ++num_characters;
            break;
        default:
            ++num_skills;
            break;
        }
    }

    CardRef drawRandom() {
        return DrawCard(cards, urand(cards.size()));
    }

    void debug() const {
        LOG("Hand (c={} s={}):", num_characters, num_skills);
        for (auto c : cards) {
            LOG("    {}", to_string(Card::Get(c)));
        }
    }
};
