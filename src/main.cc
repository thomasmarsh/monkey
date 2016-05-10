#include "log.h"
#include "moves.h"
#include "cards.h"
#include "util.h"
#include "naive.h"
#include "flatmc.h"

void Play() {
    State s(4);
    s.init();
    size_t c = 0;
    auto ragent = RandomAgent();
    auto naive = NaiveAgent();
    auto flatmc = std::make_shared<MCAgent>();
    while (!s.gameOver()) {
        LOG("");
        LOG("CHALLENGE #{}", c);
        LOG("");
        size_t r = 0;
        while (!s.challenge.finished()) {
            LOG("ROUND <{}>", r);
            while (!s.challenge.round.finished()) {
                const auto &p = s.current();
                LOG("");
                LOG("PLAYER {} TO MOVE", p.id);
                p.hand.print();
                //p.debug();
                Moves m(s);
                m.print();

                switch (s.current().id) {
                case 0: ragent.move(s); break;
                case 1: naive.move(s); break;
                case 2: naive.move(s); break; //flatmc->move(s); break;
                case 3: ragent.move(s); break;
                }
#ifndef NO_LOGGING
                for (const auto &p : s.players) {
                    LOG("Player {}", p.id);
                    p.visible.print();
                }
#endif
            }
            s.challenge.round.reset();
            if (s.gameOver()) {
                break;
            }
            ++r;
            LOG("");
        }
        //s.printScore();
        s.reset();
        ++c;
    }
    s.printScore();
}

int main() {
    chdir("/Users/tmarsh/Dropbox (Personal)/monkey");
    auto console = spd::stdout_logger_mt("console", true);
    //auto console = spd::rotating_logger_mt("console", "build/log", 1048576 * 5, 3);
    size_t q_size = 4096; //queue size must be power of 2
    spdlog::set_async_mode(q_size);
    spd::set_pattern("%H:%M:%S.%e%v");

    SET_LOG_LEVEL(info);
    Initialize();
    for (int i=0; i < 10; ++i) {
        Play();
    }
}
