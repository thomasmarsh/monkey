#pragma once

#include "round.h"

struct Challenge {
    Round round;
    bool no_styles    :1;
    bool no_weapons   :1;
    bool invert_value :1;
    bool char_bonus   :1;
    uint8_t round_num :6;

    Challenge(size_t num_players)
    : round(num_players)
    {
        reset();
    }

    void reset() {
        no_styles    = false;
        no_weapons   = false;
        invert_value = false;
        char_bonus   = false;
        round.hardReset();
        round_num = 0;
    }

    void   step()           { round.step(); }
    size_t current()  const { return round.current; }
    bool   finished() const { return round.challengeFinished(); }

#ifndef NO_DEBUG
    void debug() const {
        DLOG("Challenge:");
        DLOG("    round_num    = {}", (uint8_t)round_num);
        DLOG("    no_styles    = {}", no_styles);
        DLOG("    no_weapons   = {}", no_weapons);
        DLOG("    char_bonus   = {}", char_bonus);
        DLOG("    invert_value = {}", invert_value);
        DLOG("    current()    = {}", current());
        DLOG("    finished()   = {}", finished());
        round.debug();
    }
#endif
};
