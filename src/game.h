#pragma once

#include "log.h"
#include "moves.h"
#include "cards.h"
#include "util.h"
#include "naive.h"
#include "flatmc.h"
#include "mcts.h"

struct Game {
    State state;
    size_t challenge_num;
    size_t round_num;

    Game(uint8_t num_players=4)
    : state(num_players)
    , challenge_num(1)
    , round_num(1)
    {
        state.init();
    }

    void move() {
        RandomAgent ragent;
        NaiveAgent naive;
        MCAgent flatmc;
        MCTSAgent mcts(100);

        mcts.move(state);
        //switch (state.current().id) {
        //case 0: ragent.move(state); break;
        //case 1: mcts.move(state); break;
        //case 2: flatmc.move(state); break;
        //case 3: naive.move(state); break;
        // }
    }

    void printVisible() const {
#ifndef NO_LOGGING
        for (const auto &p : state.players) {
            LOG("Player {}", p.id);
            p.visible.print();
        }
#endif
    }

    void announcePlayer() const {
        const auto &p = state.current();
        LOG("");
        LOG("PLAYER {} TO MOVE", p.id);
        p.hand.print();
        Moves m(state);
        m.print();
    }

    void play(std::function<void()> callback=nullptr) {
        while (!state.gameOver()) {
            LOG("");
            LOG("CHALLENGE #{}", challenge_num);
            LOG("");
            while (!state.challenge.finished()) {
                LOG("ROUND <{}>", round_num);
                while (!state.challenge.round.finished()) {
                    announcePlayer();
                    move();
                    if (callback) { callback(); }
                    printVisible();
                }
                state.challenge.round.reset();
                if (state.gameOver()) {
                    break;
                }
                ++round_num;
                LOG("");
            }
            state.reset();
            ++challenge_num;
        }
        state.printScore();
    }
};
