#pragma once

#include "rand.h"
#include "bits.h"
#include "log.h"

struct Round {
    uint8_t         num_players:3;
    uint8_t         current:2;
    uint8_t         challenger:2;
    bool            challenge_finished:1;
    bool            game_over;
    Bitset<uint8_t> all;
    Bitset<uint8_t> passed;
    Bitset<uint8_t> conceded;
    Bitset<uint8_t> pending;

    Round(size_t p) : num_players(p)
    {
        game_over = false;
        all.fill(num_players);
        hardReset();
        challenger = urand(num_players);
        setCurrent();
    }

    void resetUnchecked() {
        passed = 0;
        pending = all ^ conceded;
        setCurrent();
    }

    void reset() {
        // TODO: assert instead of test
        if (!challengeFinished()) {
            resetUnchecked();
        }
    }

    void hardReset() {
        challenge_finished = false;
        conceded = 0;
        resetUnchecked();
        challenger = (challenger+1) % num_players;
        current = challenger;
        setCurrent();
    }

    void pass() {
        passed.set(current);
        if (passed == (all ^ conceded)) {
            challenge_finished = true;
        }
    }
    void concede() {
        conceded.set(current);
        if (conceded.count()+1 == num_players) {
            challenge_finished = true;
        }
    }

    void step() {
        assert(pending[current]);

        pending.clear(current);
        setCurrent();
    }

    void setCurrent() {
        while (pending && !pending[current]) {
            current = (current+1) % num_players;
        }
    }

    bool challengeFinished() const {
        return challenge_finished || game_over;
    }

    bool finished() const {
        return !pending || challengeFinished();
    }

    void debug() const {
        LOG("Round:");
        LOG("    num_players        = {}", num_players);
        LOG("    current            = {}", current);
        LOG("    all                = {:04b}", (uint8_t)all);
        LOG("    passed             = {:04b}", (uint8_t)passed);
        LOG("    conceded           = {:04b}", (uint8_t)conceded);
        LOG("    pending            = {:04b}", (uint8_t)pending);
        LOG("    challenge_finished = {}", challenge_finished);
        LOG("    finished()         = {}", finished());
    }
};
