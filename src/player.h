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
        discard_two     = false;
        score           = 0;
    }

    void placeCharacter(const Card &card)        { visible.placeCharacter(card); }
    void placeStyle(const Card &card, size_t i)  { visible.placeStyle(card, i); }
    void placeWeapon(const Card &card, size_t i) { visible.placeWeapon(card, i); }

    size_t size() const {
        return hand.size() + visible.size();
    }

#ifndef NO_DEBUG
    void debug() const {
        DLOG("PLAYER:");
        DLOG("id              = {}", id);
        DLOG("affinity        = {}", to_string(affinity));
        DLOG("ignore_affinity = {}", ignore_affinity);
        DLOG("discard_two     = {}", discard_two);
        DLOG("score           = {}", score);
        hand.debug();
        visible.debug();
    }
#endif

    struct Aggregate {
        uint64_t exposed_char;
        uint64_t exposed_style;
        uint64_t exposed_weapon;
        uint64_t double_style;

        ///  Constructor
        ///
        ///  @param ignore  player id to ignore
        ///  @param players players vector
        Aggregate(size_t ignore, const std::vector<Player> &players)
        : exposed_char(0)
        , exposed_style(0)
        , exposed_weapon(0)
        , double_style(0)
        {
            for (const auto &p : players) {
                if (p.id != ignore) {
                    auto shift = (uint64_t(p.id) << 4);
                    exposed_char   |= uint64_t(p.visible.exposed_char)   << shift;
                    exposed_style  |= uint64_t(p.visible.exposed_style)  << shift;
                    exposed_weapon |= uint64_t(p.visible.exposed_weapon) << shift;
                    double_style   |= uint64_t(p.visible.double_style)   << shift;
                }
            }
        }
    };
};
