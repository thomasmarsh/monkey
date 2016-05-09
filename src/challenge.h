#pragma once

#include "round.h"

struct Challenge {
    struct Flags {
        bool no_styles    :1;
        bool no_weapons   :1;
        bool invert_value :1;
        bool char_bonus   :1;

        Flags() { reset(); }

        void reset() {
            no_styles    = false;
            no_weapons   = false;
            invert_value = false;
            char_bonus   = false;
        }

        void debug() const {
            LOG("Flags:");
            LOG("    no_styles    = {}", no_styles);
            LOG("    no_weapons   = {}", no_weapons);
            LOG("    invert_value = {}", invert_value);
            LOG("    char_bonus   = {}", char_bonus);
        }
    };

    Round round;
    Flags flags;
    uint8_t round_num:6;

    Challenge(size_t num_players)
    : round(num_players)
    {
        reset();
    }

    void reset() {
        flags.reset();
        round.hardReset();
        round_num = 0;
    }

    void   step()           { round.step(); }
    size_t current()  const { return round.current; }
    bool   finished() const { return round.challengeFinished(); }

    void debug() const {
        LOG("Challenge:");
        LOG("    round_num  = {}", (uint8_t)round_num);
        LOG("    current()  = {}", current());
        LOG("    finished() = {}", finished());
        flags.debug();
        round.debug();
    }
};
