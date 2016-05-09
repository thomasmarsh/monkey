#pragma once

#include "hand.h"
#include "visible.h"

struct Player {
    size_t   id              :3;
    Affinity affinity        :3;
    bool     ignore_affinity :1;
    bool     discard_two     :1;
    int      score           :10;

    PlayerHand    hand;
    PlayerVisible visible;

    Player(size_t i) : id(i) { reset(); }

    void reset() {
        affinity        = Affinity::NONE;
        ignore_affinity = false;
    }

    void placeCharacter(const Card &card) { visible.placeCharacter(card); }
    void placeStyle(const Card &card, size_t i) { visible.placeStyle(card, i); }
    void placeWeapon(const Card &card, size_t i) { visible.placeWeapon(card, i); }

    size_t size() const {
        return hand.size() + visible.size();
    }

    void debug() const {
        hand.debug();
    }

    struct Aggregate {
        using u64 = uint64_t;

        static std::tuple<u64,u64,u64> Exposed(size_t ignore, const std::vector<Player> &players) {
            u64 c = 0;
            u64 s = 0;
            u64 w = 0;
            for (const auto &p : players) {
                if (p.id != ignore) {
                    c |= u64(p.visible.exposed_char) << (u64(p.id) << 4);
                    s |= u64(p.visible.exposed_style) << (u64(p.id) << 4);
                    w |= u64(p.visible.exposed_weapon) << (u64(p.id) << 4);
                }
            }
            return {c, s, w};
        }
    };
};
