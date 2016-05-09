#pragma once

#include "state.h"

struct ViewCard {
    size_t id;
    int rotation;

    ViewCard() : id(-1), rotation(0) {}
    ViewCard(size_t i) : id(-1), rotation(0) { update(i); }

    void update(size_t i) {
        if (i != id) {
            id = i;
            rotation = RandFloat() * 4 - 2;
        }
    }
};

template <typename V, typename U>
static void ResizeVector(size_t target, U &r) {
    if (target < r.size()) {
        r.clear();
    }

    while (target > r.size()) {
        r.emplace_back(V());
    }
}

struct ViewCharacter {
    ViewCard character;
    std::vector<ViewCard> styles;
    std::vector<ViewCard> weapons;

    void update(const PlayerVisible::Character &c) {
        if (c.character != character.id) {
            character.update(c.character);
        }

        ResizeVector<ViewCard>(c.styles.size(), styles);
        ResizeVector<ViewCard>(c.weapons.size(), weapons);

        for (size_t i=0; i < styles.size(); ++i) {
            styles[i].update(c.styles[i]);
        }
        for (size_t i=0; i < weapons.size(); ++i) {
            weapons[i].update(c.weapons[i]);
        }
    }
};

struct ViewPlayer {
    std::vector<ViewCharacter> characters;

    void update(const Player &p) {
        const auto &v = p.visible;
        ResizeVector<ViewCharacter>(v.characters.size(), characters);
        for (size_t i = 0; i < characters.size(); ++i) {
            characters[i].update(v.characters[i]);
        }
    }
};

struct ViewState {
    std::vector<ViewPlayer> players;

    void update(const State &state) {
        ResizeVector<ViewPlayer>(state->players.size(), players);

        for (size_t i = 0; i < players.size(); ++i) {
            players[i].update(state.players[i]);
        }
    }
};
